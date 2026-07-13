//
// Created by Davis Polito on 10/22/24.
//

#ifndef ELECTROSYNTH_MODULESECTION_H
#define ELECTROSYNTH_MODULESECTION_H
#include "synth_section.h"
#include "PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "open_gl_background.h"
#include "open_gl_image_component.h"
#include "ProcessorBase.h"

class ModuleSection : public SynthSection {
public:
    static constexpr int kHeaderHeight = 36;
    static constexpr int kContentBottomPadding = 36;

    ModuleSection(const juce::ValueTree &, std::unique_ptr<SynthSection> editor, juce::UndoManager& um);

    virtual ~ModuleSection();
    int getPreferredHeight() const override;
    int refreshHeight();
    void setAreaSkinOverride(Skin::SectionOverride skin_override);

    void renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) override {
        SynthSection::renderOpenGlComponents(open_gl,animate);
    }

    void resized() override;

    void paintBackground(Graphics& g) override;

    void setDrawBottomSeparator(bool should_draw) {
        draw_bottom_separator_ = should_draw;
        if (bottom_separator_ != nullptr)
            bottom_separator_->setVisible(should_draw);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        DBG("mousedown");
        // Right-clicks belong to the owning lane's create menu, not drag-reorder.
        if (e.mods.isPopupMenu()) {
            if (onPopupMenu) onPopupMenu(e);
            return;
        }
        dragStartY = e.getEventRelativeTo(getParentComponent()).position.getY();
        originalBounds = getBounds();
        toFront(true);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu())
            return;
        int deltaY = e.getEventRelativeTo(getParentComponent()).position.getY() - dragStartY;
        setTopLeftPosition(originalBounds.getX(), originalBounds.getY() + deltaY);

        DBG("b4drag");
        isDragging = true;
        if (onDragMove) onDragMove(this, getBounds());
        DBG("afterdrag");
    }

    void mouseUp(const juce::MouseEvent&) override {
        DBG("mousseup");
        // if(
        if (isDragging == true&&onDragEnd) onDragEnd(this, getBounds());
        DBG("afterup");
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

    std::function<void(ModuleSection*, juce::Rectangle<int>)> onDragMove;
    std::function<void(ModuleSection*, juce::Rectangle<int>)> onDragEnd;
    std::function<void(const juce::MouseEvent&)> onPopupMenu;

    int dragStartY = 0;
    juce::Rectangle<int> originalBounds;
    bool hover_;
    int height = 100;

private:
    bool isDragging = false;
    bool draw_bottom_separator_ = false;
    juce::Image background_image_;
    std::unique_ptr<SynthSection> _view;
    std::vector<Listener*> listeners_;
    juce::UndoManager& undo;
};

#endif //ELECTROSYNTH_MODULESECTION_H
