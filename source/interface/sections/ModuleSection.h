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

class ModuleSection : public SynthSection
{
public:
    static constexpr int kHeaderHeight = 36;

    ModuleSection(const juce::ValueTree &, std::unique_ptr<SynthSection> editor, juce::UndoManager& um);

    virtual ~ModuleSection();
    int getPreferredHeight() const override;
    int refreshHeight();
    void repaintModuleBackground()
    {
        background_->lock();
        background_image_ = juce::Image(juce::Image::RGB, getWidth(),getHeight(), true);
        juce::Graphics g(background_image_);
        // if (prep_view.get() != nullptr)
            paintChildBackground(g, this);
        background_->updateBackgroundImage(background_image_);
        background_->unlock();
    }
    void renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) override {
        if(!background_->isInit) {
        background_->init(open_gl);
        }
        // background_->render(open_gl);

        SynthSection::renderOpenGlComponents(open_gl,animate);
    }
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
    void mouseDown(const juce::MouseEvent& e) override
    {
        DBG("mousedown");
        dragStartY = e.getEventRelativeTo(getParentComponent()).position.getY();
        originalBounds = getBounds();
        toFront(true);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        int deltaY = e.getEventRelativeTo(getParentComponent()).position.getY() - dragStartY;
        setTopLeftPosition(originalBounds.getX(), originalBounds.getY() + deltaY);

        DBG("b4drag");
        isDragging = true;
        if (onDragMove) onDragMove(this, getBounds());
        DBG("afterdrag");
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        DBG("mousseup");
        // if(
        if (isDragging == true&&onDragEnd) onDragEnd(this, getBounds());
        DBG("afterup");
        isDragging = false;
    }
    juce::ValueTree state;
    void buttonClicked(juce::Button* clicked_button) override;
    std::unique_ptr<OpenGlShapeButton> exit_button_;
    std::shared_ptr<PlainTextComponent> title_text_;
    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void mouseEnter(const juce::MouseEvent& e) {
        hover_ = true;
    }
    void mouseExit(const juce::MouseEvent& e) override
    {
        hover_ = false;
    }
    std::function<void(ModuleSection*, juce::Rectangle<int>)> onDragMove;
    std::function<void(ModuleSection*, juce::Rectangle<int>)> onDragEnd;

    int dragStartY = 0;
    juce::Rectangle<int> originalBounds;
    bool hover_;
    int height = 100;
private:
    bool isDragging = false;
    juce::Image background_image_;
    std::unique_ptr<SynthSection> _view;
    std::vector<Listener*> listeners_;
    juce::UndoManager& undo;
};

#endif //ELECTROSYNTH_MODULESECTION_H
