//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_MODULATIONSECTION_H
#define ELECTROSYNTH_MODULATIONSECTION_H


    #include "synth_section.h"
    #include "PluginStateImpl_.h"
    #include "ParameterView/ParametersView.h"
    #include <juce_gui_basics/juce_gui_basics.h>
class ModulationButton;
class ModulationManager;
class ModulationSection : public SynthSection
{
public:
    ModulationSection(  const juce::ValueTree &, std::unique_ptr<SynthSection> editor, juce::UndoManager& um);
    virtual ~ModulationSection();

    void paintBackground(Graphics& g) override;
    // void setParametersViewEditor(electrosynth::ParametersViewEditor&&);
    // void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    juce::String getModulatorType() const;
    //void setActive(bool active) override;
    //void sliderValueChanged(Slider* changed_slider) override;
    //void setAllValues(vital::control_map& controls) override;
    //void setFilterActive(bool active);
    juce::ValueTree state;
    void addModButtonListener(ModulationManager*);
    void buttonClicked(juce::Button* clicked_button) override;
    
    ModulationButton* getModulationButton() const { return mod_button.get(); }
    std::shared_ptr<ModulationButton> getModulationButtonPtr() const { return mod_button; }

private:

    std::unique_ptr<SynthSection> _view;
    std::shared_ptr<ModulationButton> mod_button;
    std::shared_ptr<OpenGlShapeButton> exit_button_;
    juce::UndoManager& undo;
};

#endif //ELECTROSYNTH_MODULATIONSECTION_H
