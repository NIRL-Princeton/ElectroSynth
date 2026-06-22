//
// Created by Davis Polito on 11/19/24.
//

#pragma once
#include "modules_interface.h"
class ModuleSection;
class ProcessorBase;
class ModulationManager;
#include "ModuleList.h"
class EffectList;
class EffectModuleSection : public ModulesInterface<ProcessorBase>
{
public:
    explicit EffectModuleSection( ModulationManager* m, EffectList &,const juce::ValueTree &, juce::UndoManager& um);
    virtual ~EffectModuleSection();

    void setEffectPositions() override;

    PopupItems createPopupMenu() override;
    void handlePopupResult(int result) override;

    std::map<std::string, SynthSlider*> getAllSliders() override;
    std::vector<std::unique_ptr<ModuleSection>> module_sections;
    void moduleAdded(ProcessorBase* newModule) override;
    void resized() override;

    void removeModule(ProcessorBase* newModule)   override;
    void moduleListChanged() ;
    void paintBackground(Graphics &g) override;
    void redoBackgroundImage() override;
    void parentHierarchyChanged() override { redoBackgroundImage(); SynthSection::parentHierarchyChanged(); }
    std::shared_ptr<OpenGlQuad> footer_body;


    juce::ValueTree state;
    // void renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) override;
    ModuleSection* currently_dragged_;
    ModuleSection* currently_hovered_;
    // void mouseDown(const juce::MouseEvent& e) override;

    int last_dragged_index_;
    int mouse_down_y_;
    int dragged_starting_y_;
    int height;
    int reorderTargetIndex = -1; // keep track of where to insert on drop
    int placeholderIndex = -1;   // -1 means no placeholder
    int placeholderHeight = 0;
    juce::UndoManager& undo;
};

