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

MainSection::MainSection(const juce::ValueTree& v, juce::UndoManager &um, OpenGlWrapper & open_gl, SynthGuiData* data,
    ModulationManager* modulation_manager, AudioRoutingManager* audio_routing_manager) : SynthSection("main_section"),
    v(v), um(um), audio_routing_manager_(audio_routing_manager) {

    sound_interface = std::make_unique<AudioChainSection>( *data->synth->processors_,modulation_manager, audio_routing_manager ,um);
    addSubSection(sound_interface.get());

    modulation_interface = std::make_unique<ModulationModuleSection>(modulation_manager,*data->synth->modulators_, um);
    addSubSection(modulation_interface.get());

    effects_section_0 = std::make_unique<EffectModuleSection>(modulation_manager, audio_routing_manager, *data->synth->effects_0,data->synth->effects_0->state,um);
    addSubSection(effects_section_0.get());
    effects_section_1 = std::make_unique<EffectModuleSection>(modulation_manager, audio_routing_manager, *data->synth->effects_1,data->synth->effects_1->state,um);
    addSubSection(effects_section_1.get());
    effects_section_2 = std::make_unique<EffectModuleSection>(modulation_manager, audio_routing_manager, *data->synth->effects_2,data->synth->effects_2->state,um);
    addSubSection(effects_section_2.get());

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
