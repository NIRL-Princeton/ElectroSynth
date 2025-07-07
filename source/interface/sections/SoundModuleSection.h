//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_SOUNDMODULESECTION_H
#define ELECTROSYNTH_SOUNDMODULESECTION_H
#include "modules_interface.h"
class ModuleSection;
class ProcessorBase;
class ModulationManager;
#include "ModuleList.h"
class SoundModuleSection : public ModulesInterface<ProcessorBase>
{
public:
    explicit SoundModuleSection( ModulationManager* m, ModuleList<ProcessorBase> &,const juce::ValueTree &);
    virtual ~SoundModuleSection();

    void setEffectPositions() override;

    PopupItems createPopupMenu() override;
    void handlePopupResult(int result) override;

    std::map<std::string, SynthSlider*> getAllSliders() override;
    std::vector<std::unique_ptr<ModuleSection>> module_sections;
    void moduleAdded(ProcessorBase* newModule) override;
    void resized() override;

    void removeModule(ProcessorBase* newModule)   override;
    void moduleListChanged() ;

    std::shared_ptr<OpenGlQuad> footer_body;
    std::unique_ptr<OpenGlShapeButton> exit_button_;

    void buttonClicked(juce::Button* clicked_button) override;
    juce::ValueTree state;
    // void renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) override;
    ModuleSection* currently_dragged_;
    ModuleSection* currently_hovered_;
    // void mouseDown(const juce::MouseEvent& e) override;

    int last_dragged_index_;
    int mouse_down_y_;
    int dragged_starting_y_;
    int height;

};

#endif //ELECTROSYNTH_SOUNDMODULESECTION_H