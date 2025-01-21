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
    ModulationSection( const juce::ValueTree &, electrosynth::ParametersView* editor);

    virtual ~ModulationSection();

    void paintBackground(Graphics& g) override;
    //    void setParametersViewEditor(electrosynth::ParametersViewEditor&&);
    // void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    //  void setActive(bool active) override;
    //void sliderValueChanged(Slider* changed_slider) override;
    //void setAllValues(vital::control_map& controls) override;
    //void setFilterActive(bool active);
    juce::ValueTree state;
    void addModButtonListener(ModulationManager*);
private:

    std::unique_ptr<electrosynth::ParametersView> _view;
    std::shared_ptr<ModulationButton> mod_button;

};

#endif //ELECTROSYNTH_MODULATIONSECTION_H
