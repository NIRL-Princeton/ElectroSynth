//
// Created by Davis Polito on 2/1/24.
//

#ifndef ELECTROSYNTH2_MAIN_SECTION_H
#define ELECTROSYNTH2_MAIN_SECTION_H

#include "synth_section.h"
#include "ParameterView/ParametersView.h"
class ModulationSection;
class SoundModuleSection;
class ModulationModuleSection;
struct SynthGuiData;
class ModulationManager;
class MasterVoiceEnvelopeSection : public SynthSection {
public:
    MasterVoiceEnvelopeSection(const juce::ValueTree& v, juce::UndoManager &um,
        OpenGlWrapper &open_gl, SynthGuiData * data, std::unique_ptr<electrosynth::ParametersView>&&);

        void resized() override;
    void paintBackground(Graphics &g) override;
    std::unique_ptr<electrosynth::ParametersView> master_voice_envelope;
    std::shared_ptr<ModulationButton> mod_button;
};
class MainSection : public SynthSection
{
public:
    class Listener {
    public:
        virtual ~Listener() { }

        //virtual void showAboutSection() = 0;
    };

    MainSection(const juce::ValueTree& v, juce::UndoManager &um, OpenGlWrapper &open_gl, SynthGuiData * data, ModulationManager* );

    void paintBackground(Graphics& g) override;
    void resized() override;

    std::map<std::string, SynthSlider*> getAllSliders() override;
    std::map<std::string, ModulationButton*> getAllModulationButtons() override;

    void addListener(Listener* listener) { listeners_.push_back(listener); }
private:
    juce::ValueTree v;
    juce::UndoManager &um;
    std::unique_ptr<SoundModuleSection> sound_interface;
    std::vector<Listener*> listeners_;
    std::unique_ptr<ModulationModuleSection> modulation_interface;
    std::unique_ptr<MasterVoiceEnvelopeSection> master_voice_envelope_section;

};

#endif //ELECTROSYNTH2_MAIN_SECTION_H
