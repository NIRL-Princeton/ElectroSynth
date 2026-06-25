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
MasterVoiceEnvelopeSection:: MasterVoiceEnvelopeSection(const juce::ValueTree& v, juce::UndoManager &um,
                                                        OpenGlWrapper &open_gl, SynthGuiData * data, std::unique_ptr<SynthSection>&& view) : SynthSection("MasterEnv"), mod_button(std::make_unique<ModulationButton>("mod_masterenv")), master_voice_envelope(std::move(view)) {
    setName("Master Voice Envelope");
    setSidewaysHeading(false);
    header_body_ = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, "master_voice_envelope_header");
    header_body_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(header_body_, true);

    header_title_ = std::make_shared<PlainTextComponent>("master_voice_envelope_title", getName());
    header_title_->setFontType(PlainTextComponent::kLight);
    header_title_->setJustification(juce::Justification::centred);
    header_title_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(header_title_);

    master_voice_envelope->setName("VCA");
    setComponentID(master_voice_envelope->getName());
    addSubSection(master_voice_envelope.get());
    if (auto* parameters = dynamic_cast<electrosynth::ParametersView*>(master_voice_envelope.get()))
        parameters->setVerticallyCenterKnobs(true);
    addModulationButton(mod_button);
    addAndMakeVisible(mod_button.get());
    mod_button->setAlwaysOnTop(true);
}

void MasterVoiceEnvelopeSection::resized() {
    const int title_width = static_cast<int>(getTitleWidth());
    const int content_height =
        std::max(0, getHeight() - title_width - ModulationModuleSection::kTabStripHeight);
    master_voice_envelope->setBounds(0, title_width, getWidth(), content_height);
    mod_button->setBounds(0, title_width, 40, 40);
    SynthSection::resized();

    header_body_->setBounds(0, 0, getWidth(), title_width);
    header_body_->setColor(findColour(Skin::kBodyHeading, true));
    header_title_->setBounds(0, 0, getWidth(), title_width);
    header_title_->setText(getName());
    header_title_->setTextSize(size_ratio_ * 14.0f);
    header_title_->setColor(findColour(Skin::kHeadingText, true));
}

void MasterVoiceEnvelopeSection::paintBackground(Graphics &g) {
    paintContainer(g);
    paintKnobShadows(g);
    paintChildrenBackgrounds(g);
    g.setColour(findColour(Skin::kBorder, true));
    paintBorder(g);
}

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

    master_voice_envelope_section = std::make_unique<MasterVoiceEnvelopeSection>(v, um, open_gl, data,std::move(data->synth->getEngine()->MasterVoiceEnvelopeProcessor->createEditor()));
    addSubSection(master_voice_envelope_section.get());
    master_voice_envelope_section->mod_button->addListener(modulation_manager);

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
    // paintChildBackground(g,master_voice_envelope_section.get());
    paintChildrenBackgrounds(g);
    // paintKnobShadows(g);
}

void MainSection::resized() {

    int height = getHeight();
    int width = getWidth();
    int padding = getPadding()*size_ratio_;


    const int bottom_row_height = static_cast<int>(size_ratio_ * 200);
    const int bottom_row_y = height - bottom_row_height;
    const int master_envelope_width = bottom_row_height + 265;
    const int master_envelope_x = width - master_envelope_width ;
    const int top_section_height = std::max(0, bottom_row_y - padding);

    int sound_interface_width = 2*width/3- padding*2;
    int all_effects_width = getWidth() - sound_interface_width;
    sound_interface->setBounds(padding, padding, sound_interface_width, top_section_height);
    effects_section_0->setBounds(sound_interface->getRight() + padding, padding, (all_effects_width-3*padding)/3, top_section_height);
    effects_section_1->setBounds(effects_section_0->getRight() + padding, padding, (all_effects_width-3*padding)/3, top_section_height);
    effects_section_2->setBounds(effects_section_1->getRight() + padding, padding, (all_effects_width-3*padding)/3, top_section_height);

    modulation_interface->setBounds(padding, bottom_row_y, master_envelope_x - 4 * padding, bottom_row_height);
    master_voice_envelope_section->setBounds(master_envelope_x, bottom_row_y, master_envelope_width - 2 * padding, bottom_row_height);

}


std::map<std::string, SynthSlider*> MainSection::getAllSliders() {
    std::map<std::string, SynthSlider*> result = sound_interface->getAllSliders();

    const auto& extraSliders = master_voice_envelope_section->getAllSliders();
    result.insert(extraSliders.begin(), extraSliders.end());

    return result;
}
std::map<std::string, ModulationButton*> MainSection::getAllModulationButtons()
{
    std::map<std::string, ModulationButton*> result = modulation_interface->getAllModulationButtons();

    const auto& extraButtons = master_voice_envelope_section->getAllModulationButtons();
    result.insert(extraButtons.begin(), extraButtons.end());
    return result;
}
