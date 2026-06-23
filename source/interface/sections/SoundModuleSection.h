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
#include "ParameterView/RoutingView.h"
class SoundModuleSection : public ModulesInterface<ProcessorBase>
{
public:
    explicit SoundModuleSection( ModulationManager* m, ModuleList<ProcessorBase> &,const juce::ValueTree &, juce::UndoManager& um);
    virtual ~SoundModuleSection();

    void setSoundModuleIndex(int index);
    int getCollapsedHeight();
    void setEffectPositions() override;

    PopupItems createPopupMenu() override;
    void handlePopupResult(int result) override;

    std::map<std::string, SynthSlider*> getAllSliders() override;
    std::vector<std::unique_ptr<ModuleSection>> module_sections;
    void moduleAdded(ProcessorBase* newModule) override;
    void paintBackground(juce::Graphics& g) override;
    void resized() override;
    void effectsScrolled(int position) override;
    void removeModule(ProcessorBase* newModule)   override;
    void moduleListChanged() ;
    void redoBackgroundImage() override;
    std::shared_ptr<OpenGlQuad> footer_body;
    std::shared_ptr<OpenGlQuad> header_body_;
    std::shared_ptr<PlainTextComponent> header_title_;
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
    int sound_module_index_ = 1;
    std::unique_ptr<RoutingView> routing_view_;
    juce::UndoManager& undo;
};

#endif //ELECTROSYNTH_SOUNDMODULESECTION_H
