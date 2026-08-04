//
// Created by Davis Polito on 11/19/24.
//

#pragma once
#include "modules_interface.h"
#include "open_gl_combobox.h"
class ModuleSection;
class ProcessorBase;
class ModulationManager;
class FxDragCoordinator;
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

    // Narrow lane-owned API for visual-only external transfer previews. All geometry
    // and scroll conversion stays here rather than leaking module-row math upward.
    void setDragCoordinator(FxDragCoordinator* coordinator) { drag_coordinator_ = coordinator; }
    juce::Rectangle<int> getContentViewportScreenBounds() const;
    juce::Rectangle<int> getExternalHostedModuleScreenBounds(int draggedHeight,
                                                             int pointerScreenY,
                                                             int pointerOffsetY,
                                                             int horizontalOffset) const;
    void setExternalTransferHighlight(bool highlighted);
    void setExternalTransferDimmed(bool dimmed);
    void beginExternalVisualHosting(ModuleSection& dragged,
                                    juce::Component& sharedContentClip);
    void restoreExternalVisualHosting(ModuleSection& dragged);
    void excludeExternalSourceFromLayout(ModuleSection& dragged);
    void updateExternalHostedModuleBounds(ModuleSection& dragged,
                                          juce::Rectangle<int> screenBounds,
                                          juce::Component& sharedContentClip);
    void restoreExternalSourcePreview(ModuleSection& dragged);
    int beginExternalTargetPreview(ModuleSection& dragged, juce::Point<int> pointerScreen);
    int updateExternalTargetHover(ModuleSection& dragged, juce::Point<int> pointerScreen);
    int beginExternalTargetVerticalDrag(ModuleSection& dragged,
                                        juce::Point<int> pointerScreen);
    int updateExternalTargetVerticalDrag(ModuleSection& dragged,
                                         juce::Point<int> pointerScreen);
    void clearExternalTargetPreview();
    void finishSameLaneDrag(ModuleSection& dragged, bool commit);
    bool transferModuleTo(EffectModuleSection& target, ProcessorBase* processor,
                          int targetEffectIndex);


    juce::ValueTree state;
    juce::UndoManager& undo;

private:
    void configureModuleSectionDragCallbacks(ModuleSection& module);
    // Drag-reorder session. UI order previews live in module_sections; the ValueTree
    // is updated exactly once on drop, and EffectList publishes the corresponding
    // identity-based audio-thread order command.
    void beginDragSession(ModuleSection* dragged);
    void updateDragSession(ModuleSection* dragged, juce::Rectangle<int> bounds);
    void updateVerticalReorderPreview(ModuleSection& dragged,
                                      juce::Rectangle<int> boundsInContainer,
                                      bool externalTarget,
                                      bool useOverlapDisplacement = false);
    ModuleSection* externalPreviewModuleAt(int logicalIndex) const;
    void endDragSession(ModuleSection* dragged);
    void clearDragSession();
    int indexOfModuleSection(const ModuleSection* section) const;
    int calculateExternalInsertionIndex(juce::Rectangle<int> boundsInContainer) const;
    void restoreTreeDerivedOrder();
    void setModuleDragScissor(ModuleSection& module, juce::Component* scissor);

    ModuleSection* dragged_module_ = nullptr;
    ModuleSection* drop_target_module_ = nullptr;
    FxDragCoordinator* drag_coordinator_ = nullptr;
    bool external_source_excluded_ = false;
    bool external_target_preview_active_ = false;
    int external_target_insertion_index_ = -1;
    int external_target_gap_height_ = 0;
    // Overlap-based hover displacement keeps its last stable gap: reversing the
    // most recent gap move requires real opposite ghost travel, not threshold
    // re-satisfaction alone (a short ghost inside a tall module meets both sides).
    int external_gap_move_centre_y_ = 0;
    int external_gap_move_direction_ = 0;
    std::shared_ptr<OpenGlQuad> external_transfer_dim_;
    std::shared_ptr<OpenGlQuad> external_transfer_highlight_;
    // Translucent FX-accent region marking where the dragged module will land.
    std::shared_ptr<OpenGlQuad> insertion_region_;
    // Drag-mode-only translucent FX-accent bands marking each boundary between
    // adjacent non-dragged modules (boundaries touching the insertion gap are
    // covered by insertion_region_ itself).
    std::shared_ptr<OpenGlMultiQuad> drag_boundary_bands_;
};
