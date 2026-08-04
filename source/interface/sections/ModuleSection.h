//
// Created by Davis Polito on 10/22/24.
//

#ifndef ELECTROSYNTH_MODULESECTION_H
#define ELECTROSYNTH_MODULESECTION_H
#include "synth_section.h"
#include <atomic>
#include "PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "open_gl_background.h"
#include "open_gl_image_component.h"
#include "ProcessorBase.h"
#include "audio_port_component.h"

class ModuleSection : public SynthSection {
public:
    static constexpr int kHeaderHeight = 36;
    static constexpr int kContentBottomPadding = 36;

    // Drag-reorder visual states, driven by the owning lane. The wrapper only renders
    // them; policy (which module is in which state) lives with the owner.
    enum class DragVisual {
        kNormal,
        kDragged,
        kDropTarget,
        kDimmed
    };

    ModuleSection(const juce::ValueTree &, electrosynth::audio::NodeDescriptor, std::unique_ptr<SynthSection> editor, juce::UndoManager& um);

    virtual ~ModuleSection();
    int getPreferredHeight() const override;
    int refreshHeight();
    void setAreaSkinOverride(Skin::SectionOverride skin_override);
    void setDragVisual(DragVisual visual);
    void setDragAccentColor(Skin::ColorId color_id) { drag_accent_color_id_ = color_id; }
    void resetDragObservation() noexcept { isDragging = false; }
    void setHorizontalDragOwnedExternally(bool externallyOwned) noexcept {
        horizontalDragOwnedExternally_ = externallyOwned;
    }

    void renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) override {
        if (!externallyVisualHosted_.load(std::memory_order_acquire))
            SynthSection::renderOpenGlComponents(open_gl, animate);
    }
    void renderAsExternalVisual(OpenGlWrapper& open_gl, bool animate) {
        SynthSection::renderOpenGlComponents(open_gl, animate);
    }
    void setExternallyVisualHosted(bool hosted) noexcept {
        externallyVisualHosted_.store(hosted, std::memory_order_release);
    }

    void resized() override;

    void paintBackground(Graphics& g) override;

    void setDrawBottomSeparator(bool should_draw) {
        draw_bottom_separator_ = should_draw;
        if (bottom_separator_ != nullptr)
            bottom_separator_->setVisible(should_draw);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        // Right-clicks belong to the owning lane's create menu, not drag-reorder.
        if (e.mods.isPopupMenu()) {
            if (onPopupMenu) onPopupMenu(e);
            return;
        }
        dragStartY = e.getEventRelativeTo(getParentComponent()).position.getY();
        originalBounds = getBounds();
        toFront(true);
        if (onDragStart)
            onDragStart(this, e.source.getScreenPosition().roundToInt());
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu())
            return;
        int deltaY = e.getEventRelativeTo(getParentComponent()).position.getY() - dragStartY;
        // Once cross-lane intent owns horizontal placement, do not race the
        // coordinator by snapping X back to the source column on every mouse event.
        const int drag_x = horizontalDragOwnedExternally_ ? getX() : originalBounds.getX();
        setTopLeftPosition(drag_x, originalBounds.getY() + deltaY);

        isDragging = true;
        if (onDragMove)
            onDragMove(this, getBounds(), e.source.getScreenPosition().roundToInt());
    }

    void mouseUp(const juce::MouseEvent& e) override {
        if (isDragging && onDragEnd)
            onDragEnd(this, getBounds(), e.source.getScreenPosition().roundToInt());
        isDragging = false;
    }

    void mouseEnter(const juce::MouseEvent& e) {
        hover_ = true;
    }

    void mouseExit(const juce::MouseEvent& e) override {
        hover_ = false;
    }

    juce::ValueTree state;
    void buttonClicked(juce::Button* clicked_button) override;
    std::unique_ptr<OpenGlShapeButton> exit_button_;
    std::shared_ptr<PlainTextComponent> title_text_;
    std::shared_ptr<OpenGlQuad> bottom_separator_;
    void addListener(Listener* listener) { listeners_.push_back(listener); }

    std::function<void(ModuleSection*, juce::Point<int>)> onDragStart;
    std::function<void(ModuleSection*, juce::Rectangle<int>, juce::Point<int>)> onDragMove;
    std::function<void(ModuleSection*, juce::Rectangle<int>, juce::Point<int>)> onDragEnd;
    std::function<void(const juce::MouseEvent&)> onPopupMenu;

    int dragStartY = 0;
    juce::Rectangle<int> originalBounds;
    bool hover_;
    int height = 100;

    const electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept {
        return audioNodeDescriptor_;
    }
    juce::String getAudioNodeId() const {
        return state.getProperty(IDs::audioNodeId).toString();
    }


private:
    bool isDragging = false;
    bool horizontalDragOwnedExternally_ = false;
    std::atomic_bool externallyVisualHosted_ { false };
    bool draw_bottom_separator_ = true;
    DragVisual drag_visual_ = DragVisual::kNormal;
    Skin::ColorId drag_accent_color_id_ = Skin::kWidgetPrimary1;
    std::shared_ptr<OpenGlQuad> body_fill_;
    std::shared_ptr<OpenGlQuad> tint_overlay_;
    std::shared_ptr<OpenGlQuad> highlight_border_;
    juce::Image background_image_;
    std::unique_ptr<SynthSection> _view;
    std::vector<Listener*> listeners_;
    juce::UndoManager& undo;
    electrosynth::audio::NodeDescriptor audioNodeDescriptor_;
    std::shared_ptr<AudioPortComponent> output_port_;
    std::shared_ptr<AudioPortComponent> input_port_;
};

#endif //ELECTROSYNTH_MODULESECTION_H
