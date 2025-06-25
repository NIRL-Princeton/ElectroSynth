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
#include "Modulators/EnvModuleProcessor.h"

MasterVoiceEnvelopeSection:: MasterVoiceEnvelopeSection(const juce::ValueTree& v, juce::UndoManager &um,
                                                        OpenGlWrapper &open_gl, SynthGuiData * data, std::unique_ptr<electrosynth::ParametersView>&& view) : SynthSection("MasterEnv"), mod_button(std::make_unique<ModulationButton>("mod_masterenv")), master_voice_envelope(std::move(view)) {
    master_voice_envelope->setName("VCA");
    setComponentID(master_voice_envelope->getName());
    addSubSection(master_voice_envelope.get());
    addModulationButton(mod_button);
    addAndMakeVisible(mod_button.get());
    mod_button->setAlwaysOnTop(true);
}

void MasterVoiceEnvelopeSection::resized() {
    int widget_margin = findValue(Skin::kWidgetMargin);
    int title_width = getTitleWidth();
    int section_height = getKnobSectionHeight();

    Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
    Rectangle<int> knobs_area = getDividedAreaBuffered(bounds, 2, 1, widget_margin);
    Rectangle<int> settings_area = getDividedAreaUnbuffered(bounds, 4, 0, widget_margin);
    master_voice_envelope->setBounds(getLocalBounds());
    mod_button->setBounds(0, 0,40,40);
    SynthSection::resized();

}
void MasterVoiceEnvelopeSection::paintBackground(Graphics &g) {
    SynthSection::paintBackground(g);
}
#include "modulation_manager.h"
#include "FullInterface.h"
MainSection::MainSection(const juce::ValueTree& v, juce::UndoManager &um, OpenGlWrapper & open_gl, SynthGuiData* data, ModulationManager* modulation_manager) : SynthSection("main_section"), v(v), um(um)
{
    sound_interface = std::make_unique<SoundModuleSection>( modulation_manager,*data->synth->processors_);
    addSubSection(sound_interface.get());
    modulation_interface = std::make_unique<ModulationModuleSection>(modulation_manager,*data->synth->modulators_);
    addSubSection(modulation_interface.get());

    master_voice_envelope_section = std::make_unique<MasterVoiceEnvelopeSection>(v, um, open_gl, data,std::move(data->synth->getEngine()->MasterVoiceEnvelopeProcessor->createEditor()));
    addSubSection(master_voice_envelope_section.get());
    master_voice_envelope_section->mod_button->addListener(modulation_manager);
    sound_interface->onExpandChanged = [this]{resized();sound_interface->redoBackgroundImage();
                                                    auto full =findParentComponentOfClass<FullInterface>();
    full->redoBackground();};

    modulation_interface->onExpandChanged = [this]{resized();};
    //addAndMakeVisible(constructionPort);
//    ValueTree t(IDs::PREPARATION);
//
//    t.setProperty(IDs::type,electrosynth::BKPreparationType::PreparationTypeDirect, nullptr);
//    t.setProperty(IDs::x,255, nullptr);
//    t.setProperty(IDs::y,255, nullptr);
//    v.addChild(t,-1, nullptr);

    //s->setAlwaysOnTop(true);
//    test_ = std::make_unique<TestSection>();
//    addSubSection(test_.get());
    setSkinOverride(Skin::kNone);
}

void MainSection::paintBackground(juce::Graphics& g)
{
    paintBody(g);


    // paintChildBackground(g,master_voice_envelope_section.get());
    paintChildrenBackgrounds(g);
    // paintKnobShadows(g);



}

void MainSection::resized()
{

    int height = getHeight();
    int width = getWidth();
    int widget_margin = findValue(Skin::kWidgetMargin);
    int large_padding = findValue(Skin::kLargePadding);
    int padding = getPadding()*size_ratio_;
    int active_width = getWidth() - padding;
    int width_left = (active_width - padding) / 2;
    int width_right = active_width - width_left;
    int right_x = width_left + padding;

    sound_interface->setBounds(padding, padding, width- padding*2,height-200);
//     test_->setBounds(0,0,width,height - 200);
    modulation_interface->setBounds(0,height -size_ratio_* 200+ padding, width - size_ratio_*200 , size_ratio_* 200);
    master_voice_envelope_section->setBounds(width-size_ratio_*200 + padding,height -size_ratio_* 200 +padding,size_ratio_*200,size_ratio_*200);
    //constructionPort.setBounds(large_padding, 0,getDisplayScale()* width, getDisplayScale() * height);
    //constructionPort.setBounds(large_padding, 0,width, height);

    //SynthSection::resized();
}

std::map<std::string, SynthSlider*> MainSection::getAllSliders()
{
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