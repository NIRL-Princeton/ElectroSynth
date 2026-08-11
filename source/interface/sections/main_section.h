//
// Created by Davis Polito on 2/1/24.
//

#ifndef ELECTROSYNTH2_MAIN_SECTION_H
#define ELECTROSYNTH2_MAIN_SECTION_H

#include "synth_section.h"
#include "ParameterView/ParametersView.h"
#include "AudioChainSection.h"
#include "ModuleList.h"
#include "EffectsModuleSection.h"
#include "FxDragCoordinator.h"
#include <array>
class ModulationSection;
class SoundModuleSection;
class ModulationModuleSection;
struct SynthGuiData;
class MappingManager;
class EffectList;
class MasterVoiceEnvelopeSection : public SynthSection {
public:
    MasterVoiceEnvelopeSection(const juce::ValueTree& v, juce::UndoManager &um,
        OpenGlWrapper &open_gl, SynthGuiData * data, std::unique_ptr<SynthSection>&&);

    void resized() override;
    void paintBackground(Graphics &g) override;
    std::unique_ptr<SynthSection> master_voice_envelope;
    std::shared_ptr<ConnectionButton> mod_button;
    std::shared_ptr<OpenGlQuad> header_body_;
    std::shared_ptr<PlainTextComponent> header_title_;
};
class MainSection : public SynthSection
{
public:
    class Listener {
    public:
        virtual ~Listener() { }

        //virtual void showAboutSection() = 0;
    };

    MainSection(const juce::ValueTree& v, juce::UndoManager &um, OpenGlWrapper &open_gl, SynthGuiData * data, MappingManager*);
    ~MainSection() override;

    void paintBackground(Graphics& g) override;
    void resized() override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;

    std::map<std::string, SynthSlider*> getAllSliders() override;
    std::map<std::string, ConnectionButton*> getAllModulationButtons() override;

    void addListener(Listener* listener) { listeners_.push_back(listener); }


private:
    juce::ValueTree v;
    juce::UndoManager &um;
    std::unique_ptr<AudioChainSection> sound_interface;
    std::unique_ptr<EffectModuleSection> effects_section_1;
    std::unique_ptr<EffectModuleSection> effects_section_2;
    std::unique_ptr<EffectModuleSection> effects_section_0;
    std::vector<Listener*> listeners_;
    std::unique_ptr<ModulationModuleSection> modulation_interface;
    std::unique_ptr<MasterVoiceEnvelopeSection> master_voice_envelope_section;
    std::unique_ptr<juce::Component> fx_drag_clip_;
    std::unique_ptr<FxDragCoordinator> fx_drag_coordinator_;
    std::array<EffectList*, 3> effect_lists_ { nullptr, nullptr, nullptr };

};

#endif //ELECTROSYNTH2_MAIN_SECTION_H
