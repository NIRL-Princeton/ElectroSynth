//
// Created by Davis Polito on 11/19/24.
//

#pragma once
#include "modules_interface.h"
#include "open_gl_combobox.h"
#include "ModuleList.h"
class ModuleSection;
class ProcessorBase;
class MappingManager;
class EffectList;
class AudioRoutingManager;

class EffectModuleSection : public ModulesInterface<ProcessorBase> {
public:
    explicit EffectModuleSection( MappingManager* m, AudioRoutingManager* ,EffectList &,const juce::ValueTree &, juce::UndoManager& um);
    virtual ~EffectModuleSection();

    void setEffectPositions() override;

    PopupItems createPopupMenu() override;
    void handlePopupResult(int result) override;
    void buttonClicked(juce::Button* button) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    std::map<std::string, SynthSlider*> getAllSliders() override;
    std::vector<std::unique_ptr<ModuleSection>> module_sections;
    void moduleAdded(ProcessorBase* newModule) override;
    void resized() override;

    void removeModule(ProcessorBase* newModule)   override;
    void moduleListChanged() ;
    void moduleOrderChanged() override;
    void paintBackground(Graphics &g) override;
    void redoBackgroundImage() override;
    void parentHierarchyChanged() override { redoBackgroundImage(); SynthSection::parentHierarchyChanged(); }
    std::shared_ptr<OpenGlQuad> footer_body;
    std::shared_ptr<OpenGlQuad> header_body_;
    std::shared_ptr<PlainTextComponent> header_title_;
    std::unique_ptr<OpenGLComboBox> routing_combo_box_;
    std::unique_ptr<OpenGlShapeButton> add_effect_button_;
    std::shared_ptr<OpenGlImageComponent> add_effect_button_background_;
    // Coincident GL overlays reproduce the Modulation panel's twice-painted 1.0 outline
    // while keeping the FX perimeter above its scroll image and module content.
    std::shared_ptr<OpenGlQuad> border_overlay_;
    std::shared_ptr<OpenGlQuad> border_overlay_second_pass_;


    juce::ValueTree state;
    juce::UndoManager& undo;
    AudioRoutingManager* audio_routing_manager_ = nullptr;

private:
    // Drag-reorder session. UI order previews live in module_sections; the ValueTree
    // is updated exactly once on drop. DSP processing order is intentionally not
    // updated yet (deferred) — it re-syncs to tree order on preset/state reload.
    void beginDragSession(ModuleSection* dragged);
    void updateDragSession(ModuleSection* dragged, juce::Rectangle<int> bounds);
    void endDragSession(ModuleSection* dragged);
    void clearDragSession();
    int indexOfModuleSection(const ModuleSection* section) const;

    ModuleSection* dragged_module_ = nullptr;
    ModuleSection* drop_target_module_ = nullptr;
    // Translucent FX-accent region marking where the dragged module will land.
    std::shared_ptr<OpenGlQuad> insertion_region_;
    // Drag-mode-only translucent FX-accent bands marking each boundary between
    // adjacent non-dragged modules (boundaries touching the insertion gap are
    // covered by insertion_region_ itself).
    std::shared_ptr<OpenGlMultiQuad> drag_boundary_bands_;

    std::shared_ptr<AudioPortComponent> lane_input_port_;
    std::shared_ptr<AudioPortComponent> lane_output_port_;

    std::unique_ptr<AudioConnectionSlots> lane_input_slots_;
    std::unique_ptr<AudioConnectionSlots> lane_output_slots_;
};
