//
// Created by Davis Polito on 2/1/24.
//
#include "main_section.h"
#include "synth_gui_interface.h"
#include "synth_slider.h"
#include "SoundModuleSection.h"
#include "ModulationModuleSection.h"
#include "synth_base.h"
#include "ModulationSection.h"
#include "modulation_button.h"
#include "sound_engine.h"
#include "sound_engine.h"
#include "sound_engine.h"
#include "Modulators/EnvModuleProcessor.h"
#include "EffectList.h"
#include "modulation_manager.h"
#include "FullInterface.h"

MainSection::MainSection(const juce::ValueTree& v, juce::UndoManager &um, OpenGlWrapper & open_gl,
    SynthGuiData* data, ModulationManager* modulation_manager) : SynthSection("main_section"), v(v), um(um) {

    sound_interface = std::make_unique<AudioChainSection>( *data->synth->processors_,modulation_manager, um);
    addSubSection(sound_interface.get());

    modulation_interface = std::make_unique<ModulationModuleSection>(modulation_manager,*data->synth->modulators_, um);
    addSubSection(modulation_interface.get());

    effects_section_0 = std::make_unique<EffectModuleSection>(modulation_manager, *data->synth->effects_0,data->synth->effects_0->state,um);
    addSubSection(effects_section_0.get());
    effects_section_1 = std::make_unique<EffectModuleSection>(modulation_manager, *data->synth->effects_1,data->synth->effects_1->state,um);
    addSubSection(effects_section_1.get());
    effects_section_2 = std::make_unique<EffectModuleSection>(modulation_manager, *data->synth->effects_2,data->synth->effects_2->state,um);
    addSubSection(effects_section_2.get());

    // Shared, mouse-transparent visual layer above the sibling lanes. The clip
    // component is sized to the union of their content viewports in resized().
    fx_drag_clip_ = std::make_unique<juce::Component>("fx_drag_content_clip");
    fx_drag_clip_->setInterceptsMouseClicks(false, false);
    addAndMakeVisible(fx_drag_clip_.get());

    fx_drag_coordinator_ = std::make_unique<FxDragCoordinator>(*fx_drag_clip_);
    const auto laneIdentifier = [](const EffectModuleSection& lane, int fallbackIndex) {
        const auto persisted = lane.state.getProperty(IDs::audioNodeId).toString();
        return LaneIdentifier { persisted.isNotEmpty()
                                    ? persisted
                                    : "fx-lane-" + juce::String(fallbackIndex) };
    };
    const auto lane0Id = laneIdentifier(*effects_section_0, 0);
    const auto lane1Id = laneIdentifier(*effects_section_1, 1);
    const auto lane2Id = laneIdentifier(*effects_section_2, 2);
    fx_drag_coordinator_->registerLane(*effects_section_0, lane0Id);
    fx_drag_coordinator_->registerLane(*effects_section_1, lane1Id);
    fx_drag_coordinator_->registerLane(*effects_section_2, lane2Id);

    auto* list0 = data->synth->effects_0.get();
    auto* list1 = data->synth->effects_1.get();
    auto* list2 = data->synth->effects_2.get();
    effect_lists_ = { list0, list1, list2 };
    list0->onUiTransferRequested = [this, list1](ProcessorBase* processor, EffectList& target, int index) {
        auto* targetSection = &target == list1 ? effects_section_1.get()
                            : &target == effect_lists_[2] ? effects_section_2.get() : nullptr;
        if (targetSection == nullptr)
            return false;
        return effects_section_0->transferModuleTo(*targetSection, processor, index);
    };
    list1->onUiTransferRequested = [this, list0](ProcessorBase* processor, EffectList& target, int index) {
        auto* targetSection = &target == list0 ? effects_section_0.get()
                            : &target == effect_lists_[2] ? effects_section_2.get() : nullptr;
        if (targetSection == nullptr)
            return false;
        return effects_section_1->transferModuleTo(*targetSection, processor, index);
    };
    list2->onUiTransferRequested = [this, list0](ProcessorBase* processor, EffectList& target, int index) {
        auto* targetSection = &target == list0 ? effects_section_0.get()
                            : &target == effect_lists_[1] ? effects_section_1.get() : nullptr;
        if (targetSection == nullptr)
            return false;
        return effects_section_2->transferModuleTo(*targetSection, processor, index);
    };

    fx_drag_coordinator_->onMoveRequested = [this, list0, list1, list2,
                                              lane0Id, lane1Id, lane2Id](const FxMoveIntent& intent) {
        EffectList* source = intent.sourceLane == lane0Id ? list0
                           : intent.sourceLane == lane1Id ? list1
                           : intent.sourceLane == lane2Id ? list2 : nullptr;
        EffectList* target = intent.targetLane == lane0Id ? list0
                           : intent.targetLane == lane1Id ? list1
                           : intent.targetLane == lane2Id ? list2 : nullptr;
        if (source == nullptr || target == nullptr)
            return false;
        this->um.beginNewTransaction("Move effect between lanes");
        return source->moveEffectTo(*target, intent.moduleAudioNodeId,
                                    intent.targetEffectIndex, this->um);
    };

    master_voice_envelope_section = std::make_unique<MasterVoiceEnvelopeSection>(v, um, open_gl, data,std::move(data->synth->getEngine()->MasterVoiceEnvelopeProcessor->createEditor()));
    master_voice_envelope_section->mod_button->addListener(modulation_manager);
    modulation_interface->setVCAModulationSection(master_voice_envelope_section.get(),
                                                      master_voice_envelope_section->mod_button);

    modulation_interface->onExpandChanged = [this]{resized();};
    //addAndMakeVisible(constructionPort);
//    ValueTree t(IDs::PREPARATION);
//    t.setProperty(IDs::type,electrosynth::BKPreparationType::PreparationTypeDirect, nullptr);
//    t.setProperty(IDs::x,255, nullptr);
//    t.setProperty(IDs::y,255, nullptr);
//    v.addChild(t,-1, nullptr);
    //s->setAlwaysOnTop(true);
//    test_ = std::make_unique<TestSection>();
//    addSubSection(test_.get());
    setSkinOverride(Skin::kNone);
}

MainSection::~MainSection() {
    for (auto* list : effect_lists_)
        if (list != nullptr)
            list->onUiTransferRequested = nullptr;
}

void MainSection::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
    SynthSection::renderOpenGlComponents(open_gl, animate);

    // The source lane suppresses its normal render while externally hosted. Its
    // visual wrapper is temporarily parented to fx_drag_clip_, then this renders
    // that same subtree once after all lanes. Model/processor ownership never moves.
    if (fx_drag_coordinator_ != nullptr)
        if (auto* hosted = fx_drag_coordinator_->getExternallyHostedModule())
            hosted->renderAsExternalVisual(open_gl, animate);
}

void MainSection::paintBackground(juce::Graphics& g) {
    paintBody(g);
    paintChildrenBackgrounds(g);
}

void MainSection::resized() {

    const int height = getHeight();
    const int width = getWidth();
    const int padding = static_cast<int>(getPadding() * size_ratio_);
    const int bottom_row_height = static_cast<int>(size_ratio_ * 200);
    const int content_x = padding;
    const int content_y = padding;
    const int content_width = std::max(0, width - 2 * padding);
    const int content_height = std::max(0, height - 2 * padding);
    const int modulation_y = height - padding - bottom_row_height;
    const int top_left_height = std::max(0, modulation_y - content_y - padding);

    const int left_column_width = std::max(0, (content_width - 2 * padding) / 2 - 100);
    const int fx_x = content_x + left_column_width + padding;
    const int fx_total_width = std::max(0, content_width - left_column_width - padding);
    const int fx_width = std::max(0, (fx_total_width - 2 * padding) / 3);

    sound_interface->setBounds(content_x, content_y, left_column_width, top_left_height);
    modulation_interface->setBounds(content_x, modulation_y, left_column_width, bottom_row_height);

    effects_section_0->setBounds(fx_x, content_y, fx_width, content_height);
    effects_section_1->setBounds(effects_section_0->getRight() + padding, content_y, fx_width, content_height);
    effects_section_2->setBounds(effects_section_1->getRight() + padding, content_y,
                                 std::max(0, content_x + content_width - effects_section_1->getRight() - padding),
                                 content_height);

    const int fx_header_height = static_cast<int>(effects_section_0->getTitleWidth());
    fx_drag_clip_->setBounds(fx_x, content_y + fx_header_height, fx_total_width,
                             std::max(0, content_height - fx_header_height - 2));

}


std::map<std::string, SynthSlider*> MainSection::getAllSliders() {

    std::map<std::string, SynthSlider*> result = sound_interface->getAllSliders();
    const auto& masterSliders = master_voice_envelope_section->getAllSliders();
    const auto& fx_1 = effects_section_0->getAllSliders();
    const auto& fx_2 = effects_section_1->getAllSliders();
    const auto& fx_3 = effects_section_2->getAllSliders();

    result.insert(masterSliders.begin(), masterSliders.end());
    result.insert(fx_1.begin(), fx_1.end());
    result.insert(fx_2.begin(), fx_2.end());
    result.insert(fx_3.begin(), fx_3.end());

    return result;
}
std::map<std::string, ModulationButton*> MainSection::getAllModulationButtons()
{
    std::map<std::string, ModulationButton*> result = modulation_interface->getAllModulationButtons();

    const auto& extraButtons = master_voice_envelope_section->getAllModulationButtons();
    result.insert(extraButtons.begin(), extraButtons.end());
    return result;
}
