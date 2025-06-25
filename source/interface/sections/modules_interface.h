//
// Created by Davis Polito on 10/22/24.
//

#ifndef ELECTROSYNTH_SOUND_GENERATOR_SECTION_H
#define ELECTROSYNTH_SOUND_GENERATOR_SECTION_H
#include "synth_section.h"
#include "Identifiers.h"
#include "tracktion_ValueTreeUtilities.h"
#include <string>

#include "ModuleList.h"
#include "public.sdk/source/vst/hosting/module.h"


class ModulesContainer : public SynthSection {
    public:
        ModulesContainer(String name) : SynthSection(name) {
            setInterceptsMouseClicks(false,true);
        }
    void resized() override {
            SynthSection::resized();
        }
    void paintBackground(Graphics& g) override {
        // g.fillAll(findColour(Skin::kBackground, true));
paintChildrenShadows(g);
paintChildrenBackgrounds(g);
}
};

class EffectsViewport : public juce::Viewport {
public:

    class Listener {
    public:
        virtual ~Listener() { }
        virtual void effectsScrolled(int position) = 0;
        virtual void startScroll() = 0;
        virtual void endScroll() = 0;
    };

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &wheel) override {
        for (Listener* listener : listeners_)
            listener->startScroll();

        Viewport::mouseWheelMove(e, wheel);

        for (Listener* listener : listeners_)
            listener->endScroll();
    }
    void visibleAreaChanged(const juce::Rectangle<int>& visible_area) override {
        Viewport::visibleAreaChanged(visible_area);
        for (Listener* listener : listeners_) {
            if (isVerticalScrollbarOnTheRight())
                listener->effectsScrolled(visible_area.getY());
            else
                listener->effectsScrolled(visible_area.getX());
        }


    }

private:
    std::vector<Listener*> listeners_;
};
template<typename T>
class ModulesInterface : public SynthSection,
                         public juce::ScrollBar::Listener, EffectsViewport::Listener,
                        public ModuleList<T>::Listener

{
public:
    class Listener {
    public:
        virtual ~Listener() { }
        virtual void effectsMoved() = 0;
        virtual void added() =0;
        virtual void removed() = 0;
    };
//    T* createNewObject(const juce::ValueTree& v) override;
//    void deleteObject (ModuleSection* at) override;
    void reset() override;


    ModulesInterface(ModuleList<T> &);
    virtual ~ModulesInterface();

    void paintBackground(juce::Graphics& g) override;
    void paintChildrenShadows(juce::Graphics& g) override { }
    void resized() override;
    virtual void redoBackgroundImage();
    void mouseDown (const juce::MouseEvent& e) override;

    void setFocus() { grabKeyboardFocus(); }
    virtual void setEffectPositions() = 0;

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(juce::OpenGLContext& open_gl) override;

    void scrollBarMoved(ScrollBar* scroll_bar, double range_start) override;
    virtual void setScrollBarRange();

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void effectsScrolled(int position) override {
        setScrollBarRange();
        scroll_bar_->setCurrentRange(position, viewport_.getHeight());
        // DBG("position: " + String(position));
        for (Listener* listener : listeners_)
            listener->effectsMoved();
    }
    void startScroll() override {

    }
    void endScroll() override {

    }

    virtual PopupItems createPopupMenu() = 0;
    virtual void handlePopupResult(int result) = 0;
    bool isExpanded() const { return toggle_button_->getToggleState(); }
    std::function<void()> onExpandChanged;
    void buttonClicked(juce::Button* b) override
    {
        if (b == toggle_button_.get())
        {

            toggle_button_->setShape(toggle_button_->getToggleState() ? Paths::downTriangle() : Paths::upTriangle());

            resized();
            if (onExpandChanged)
                onExpandChanged();
        }
    }

protected:
    ModuleList<T>& list;
    std::vector<Listener*> listeners_;
    EffectsViewport viewport_;
    std::unique_ptr<ModulesContainer> container_;
    OpenGlImage background_;
    CriticalSection open_gl_critical_section_;

    std::unique_ptr<OpenGlScrollBar> scroll_bar_;
//
//    std::vector<std::unique_ptr<SynthSection>> modules;
    std::unique_ptr<OpenGlShapeButton> toggle_button_;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulesInterface)
};
template<typename T>
ModulesInterface<T>::ModulesInterface( ModuleList<T>& list_) : SynthSection("modules") ,list(list_) {
    setSidewaysHeading(false);
    toggle_button_ = std::make_unique<OpenGlShapeButton>("-");
    addAndMakeVisible(toggle_button_.get());
    addOpenGlComponent(toggle_button_->getGlComponent());
    toggle_button_->addListener(this);
    toggle_button_->setShape(Paths::downTriangle());
    toggle_button_->setClickingTogglesState(true);
    toggle_button_->setToggleState(true,juce::dontSendNotification);

    addToggleButton(toggle_button_.get(),true);

    toggle_button_->setClickingTogglesState(true);
    toggle_button_->setToggleState(true, juce::dontSendNotification);

    container_ = std::make_unique<ModulesContainer>("container");

    addAndMakeVisible(viewport_);
    viewport_.setViewedComponent(container_.get());
    viewport_.addListener(this);
    viewport_.setInterceptsMouseClicks(false,true);
    //breaks sacling if true
    addSubSection(container_.get(), false);

    // container_->toFront(true);
    // container_->setInterceptsMouseClicks(false,true);

    setOpaque(false);
    list.addListener(this);


}
template<typename T>
ModulesInterface<T>::~ModulesInterface() {
    list.removeListener(this);
    //freeObjects(;

}
template<typename T>
void ModulesInterface<T>::paintBackground(Graphics& g) {
    g.setColour(Colours::purple);
    // Colour background = findColour(Skin::kBackground, true);
    // g.setColour(background);
    // g.fillRect(getLocalBounds().withRight(getWidth() - findValue(Skin::kLargePadding) / 2));

    g.fillRoundedRectangle(getLocalBounds().toFloat(), findValue(Skin::kBodyRounding));

    int body_rounding = findValue(Skin::kBodyRounding);
    g.setColour(Colours::red);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), body_rounding, 1.0f);
   // paintContainer(g);
    paintHeadingText(g);

    // paintChildrenBackgrounds(g);
    paintBorder(g);
    // paintChildBackground(g,container_.get());

    redoBackgroundImage();
}
template<typename T>
void ModulesInterface<T>::redoBackgroundImage() {
    Colour background = findColour(Skin::kBackground, true);

    int height = std::max(container_->getHeight(), getHeight());
    int width = std::max(container_->getWidth(), getWidth());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
    Image background_image = Image(Image::ARGB, width * mult, height * mult, true);

    Graphics background_graphics(background_image);
    background_graphics.addTransform(AffineTransform::scale(mult));
    background_graphics.fillAll(background);
    container_->paintBackground(background_graphics);
    background_.setOwnImage(background_image);
}
template<typename T>
void ModulesInterface<T>::resized() {
    static constexpr float kEffectOrderWidthPercent = 0.2f;

    ScopedLock lock(open_gl_critical_section_);

    int order_width = getWidth() * kEffectOrderWidthPercent;
    //    effect_order_->setBounds(0, 0, order_width, getHeight());
    //    effect_order_->setSizeRatio(size_ratio_);
    int large_padding = findValue(Skin::kLargePadding);
    int shadow_width = getComponentShadowWidth();
    int viewport_x = 0 + large_padding - shadow_width;
    int viewport_width = getWidth() - viewport_x - large_padding + 2 * shadow_width;
    auto area = getLocalBounds();
    auto header = area.removeFromTop(30);
    toggle_button_->setBounds(0,0,getTitleWidth(),getTitleWidth());
    if (isExpanded()) {
        viewport_.setBounds(0,0,getWidth(),getHeight()); //getHeight()-getTitleWidth() - (large_padding + 20 * shadow_width));
        setEffectPositions();
        scroll_bar_->setBounds(getWidth() - large_padding + 1, getTitleWidth() + large_padding, large_padding - 2, getHeight() -getTitleWidth()-(large_padding + 2 * shadow_width));
        scroll_bar_->setColor(findColour(Skin::kLightenScreen, true));


    }
    else
    {
        viewport_.setBounds(0,0,0,0);
        container_->setBounds(0,0,0,0);
    }

    SynthSection::resized();
}



template<typename T>
void ModulesInterface<T>::mouseDown (const juce::MouseEvent& e)
{
    if(e.mods.isPopupMenu())
    {
        PopupItems options = createPopupMenu();
        showPopupSelector(this, e.getPosition(), options, [=](int selection) { handlePopupResult(selection); });
    }
    juce::Component::mouseDown(e);
}



template<typename T>
void ModulesInterface<T>::initOpenGlComponents(OpenGlWrapper& open_gl) {
    background_.init(open_gl);
    SynthSection::initOpenGlComponents(open_gl);
}

template<typename T>
void ModulesInterface<T>::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
    ScopedLock lock(open_gl_critical_section_);

    OpenGlComponent::setViewPort(&viewport_, open_gl);

    float image_width = background_.getImageWidth(); //electrosynth::utils::nextPowerOfTwo(background_.getImageWidth());
    float image_height =background_.getImageHeight(); // electrosynth::utils::nextPowerOfTwo(background_.getImageHeight());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
    float width_ratio = image_width / (container_->getWidth() * mult);
    float height_ratio = image_height / (viewport_.getHeight() * mult);
   // DBG(viewport_.getViewPositionY());
    float y_offset =(2.0f * viewport_.getViewPositionY()) /getHeight();

    // --- Debug output ---
    // DBG("image_width: " + juce::String(image_width));
    // DBG("image_height: " + juce::String(image_height));
    // DBG("mult (scale factor): " + juce::String(mult));
    // DBG("container width: " + juce::String(container_->getWidth()));
    // DBG("viewport height: " + juce::String(viewport_.getHeight()));
    // DBG("width_ratio: " + juce::String(width_ratio));
    // DBG("height_ratio: " + juce::String(height_ratio));
    // DBG("viewport Y offset: " + juce::String(viewport_.getViewPositionY()));
    // DBG("computed y_offset: " + juce::String(y_offset));
    //
    background_.setTopLeft(-1.0f, 1.0f+ y_offset);
    background_.setTopRight(-1.0f + 2.0f * width_ratio,  1.0f+y_offset);
    background_.setBottomLeft(-1.0f, 1.0f - 2.0f * height_ratio + y_offset);
    background_.setBottomRight(-1.0f + 2.0f * width_ratio, 1.0f - 2.0f * height_ratio + y_offset);
    background_.setColor(Colours::white);
    background_.drawImage(open_gl);
    OpenGlComponent::setScissorBounds(this, viewport_.getBounds(),open_gl);
    SynthSection::renderOpenGlComponents(open_gl, animate);
    // DBG("TopLeft: (" + juce::String(-1.0f) + ", " + juce::String(1.0f + y_offset) + ")");
    // DBG("TopRight: (" + juce::String(-1.0f + 2.0f * width_ratio) + ", " + juce::String(1.0f + y_offset) + ")");
    // DBG("BottomLeft: (" + juce::String(-1.0f) + ", " + juce::String(1.0f - 2.0f * height_ratio + y_offset) + ")");
    // DBG("BottomRight: (" + juce::String(-1.0f + 2.0f * width_ratio) + ", " + juce::String(1.0f - 2.0f * height_ratio + y_offset) + ")");

}
template<typename T>
void ModulesInterface<T>::destroyOpenGlComponents(juce::OpenGLContext& open_gl) {
    background_.destroy(open_gl);
    SynthSection::destroyOpenGlComponents(open_gl);
}
template<typename T>
void ModulesInterface<T>::scrollBarMoved(ScrollBar* scroll_bar, double range_start) {
    viewport_.setViewPosition(juce::Point<int>(0, std::ceil(range_start)));
    DBG(range_start);
}
template<typename T>
void ModulesInterface<T>::setScrollBarRange() {
    scroll_bar_->setRangeLimits(0.0, container_->getHeight());
    scroll_bar_->setCurrentRange(scroll_bar_->getCurrentRangeStart(), viewport_.getHeight(), dontSendNotification);
 //   DBG("container height: " + String(container_->getHeight()));
  //  DBG("viewport height: " + String(viewport_.getHeight()));
   // DBG("scrollbar range: " + String(scroll_bar_->getCurrentRangeStart()) );
}
#include "synth_gui_interface.h"
#include "synth_base.h"
template<typename T>
void ModulesInterface<T>:: reset() {
    SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();
    if (_parent != nullptr)
        list.setValueTree( _parent->getSynth()->tree.getChildWithName(IDs::CHAINS));
    SynthSection::reset();
}
#endif //ELECTROSYNTH_SOUND_GENERATOR_SECTION_H
