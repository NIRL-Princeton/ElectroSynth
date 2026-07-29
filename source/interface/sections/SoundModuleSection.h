//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_SOUNDMODULESECTION_H
#define ELECTROSYNTH_SOUNDMODULESECTION_H
#include "modules_interface.h"
#include "ModuleList.h"
#include "ParameterView/RoutingView.h"

class ModuleSection;
class ProcessorBase;
class ModulationManager;
class AudioRoutingManager;

class SoundModuleSection : public ModulesInterface<ProcessorBase> {
public:
    explicit SoundModuleSection( ModulationManager* m, AudioRoutingManager*, ModuleList<ProcessorBase> &,const juce::ValueTree &, juce::UndoManager& um);
    virtual ~SoundModuleSection();

    void setSoundModuleIndex(int index);
    int getCollapsedHeight();
    int getExpandedHeight();
    void setEffectPositions() override;

    PopupItems createPopupMenu() override;
    void handlePopupResult(int result) override;

    std::map<std::string, SynthSlider*> getAllSliders() override;
    std::vector<std::unique_ptr<ModuleSection>> module_sections;
    void moduleAdded(ProcessorBase* newModule) override;
    void paintBackground(juce::Graphics& g) override;
    void resized() override;
    void removeModule(ProcessorBase* newModule)   override;
    void moduleListChanged() ;
    void redoBackgroundImage() override;
    std::shared_ptr<OpenGlQuad> footer_body;
    std::shared_ptr<OpenGlQuad> header_body_;
    std::shared_ptr<PlainTextComponent> header_title_;
    std::unique_ptr<OpenGlShapeButton> exit_button_;

    std::unique_ptr<OpenGlShapeButton> add_to_module_button_;
    std::shared_ptr<OpenGlQuad> add_button_background_;
    void mouseEnter(const MouseEvent& event) override;
    void mouseExit(const MouseEvent& event) override;

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
    AudioRoutingManager* audio_routing_manager_ = nullptr;
};

#endif //ELECTROSYNTH_SOUNDMODULESECTION_H
