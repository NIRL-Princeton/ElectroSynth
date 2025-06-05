//
// Created by Davis Polito on 2/1/24.
//

#ifndef ELECTROSYNTH2_MAIN_SECTION_H
#define ELECTROSYNTH2_MAIN_SECTION_H

#include "synth_section.h"


class TestSection;
class SoundModuleSection;
class ModulationModuleSection;
struct SynthGuiData ;
class ModulationManager;
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
    std::unique_ptr<TestSection> test_;
    juce::ValueTree v;
    juce::UndoManager &um;
    std::unique_ptr<SoundModuleSection> sound_interface;
    std::vector<Listener*> listeners_;
    std::unique_ptr<ModulationModuleSection> modulation_interface;
};

#endif //ELECTROSYNTH2_MAIN_SECTION_H
