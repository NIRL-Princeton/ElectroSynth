//
// Created by Davis Polito on 10/22/24.
//

#ifndef ELECTROSYNTH_MODULESECTION_H
#define ELECTROSYNTH_MODULESECTION_H
#include "synth_section.h"
#include "PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include "ProcessorBase.h"

class ModuleSection : public SynthSection
{
public:
    ModuleSection(const juce::ValueTree &, electrosynth::ParametersView* editor);

    virtual ~ModuleSection();

    void paintBackground(Graphics& g) override;
//    void setParametersViewEditor(electrosynth::ParametersViewEditor&&);
    // void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
  //  void setActive(bool active) override;
    //void sliderValueChanged(Slider* changed_slider) override;
    //void setAllValues(vital::control_map& controls) override;
    //void setFilterActive(bool active);
//    void mouseEnter (const MouseEvent& event)
//    {
//        DBG("mouseenter doulesection");
//    }
    juce::ValueTree state;
private:
    ProcessorBase* processor;
    std::unique_ptr<electrosynth::ParametersView> _view;


};

#endif //ELECTROSYNTH_MODULESECTION_H
