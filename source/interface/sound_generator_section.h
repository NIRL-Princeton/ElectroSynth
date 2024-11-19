//
// Created by Davis Polito on 10/22/24.
//

#ifndef ELECTROSYNTH_SOUND_GENERATOR_SECTION_H
#define ELECTROSYNTH_SOUND_GENERATOR_SECTION_H
#include "synth_section.h"
#include "Identifiers.h"
#include <tracktion_engine.h>
#include <functional>
#include <map>
#include <string>
#include <iostream>
#include "Factory.h"
//template <class Base>
//class Factory {
//public:
//    using CreateFunction = std::function<Base*(std::any)>;
//
//    template <typename T, typename... Args>
//    void registerType(const std::string& typeName) {
//        creators[typeName] = [](std::any args) -> Base* {
//            try {
//                auto tupleArgs = std::any_cast<std::tuple<Args...>>(args); // Unpack std::any into tuple
//                return std::apply([](auto&&... unpackedArgs) {
//                    return new T(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);  // Forward arguments to constructor
//                }, tupleArgs);  // Apply the arguments
//            } catch (const std::bad_any_cast& e) {
//                std::cerr << "std::bad_any_cast: " << e.what() << " (expected tuple)" << std::endl;
//                return nullptr;
//            }
//        };
//    }
//
//    // Create object with arguments wrapped in std::any
//    Base* create(const std::string& typeName, std::any args) const {
//        auto it = creators.find(typeName);
//        if (it != creators.end()) {
//            return it->second(args);  // Call the creation function with arguments
//        }
//        return nullptr;  // Type not found
//    }
//
//
//private:
//    std::map<std::string, CreateFunction> creators;
//};


class ModulesContainer : public SynthSection {
    public:
        ModulesContainer(String name) : SynthSection(name) {  }
    void paintBackground(Graphics& g) override {
        g.fillAll(findColour(Skin::kBackground, true));
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
    };

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void visibleAreaChanged(const juce::Rectangle<int>& visible_area) override {
        for (Listener* listener : listeners_)
            listener->effectsScrolled(visible_area.getY());

        Viewport::visibleAreaChanged(visible_area);
    }

private:
    std::vector<Listener*> listeners_;
};
template<typename T>
class ModulesInterface : public SynthSection,
                         public juce::ScrollBar::Listener, EffectsViewport::Listener,
     public tracktion::engine::ValueTreeObjectList<T>
{
public:
    class Listener {
    public:
        virtual ~Listener() { }
        virtual void effectsMoved() = 0;
    };
//    T* createNewObject(const juce::ValueTree& v) override;
//    void deleteObject (ModuleSection* at) override;


    // void reset() override;
    void newObjectAdded (T*) override;
    void objectRemoved (T*) override     { resized();}//resized(); }
    void objectOrderChanged() override              {resized(); }//resized(); }
    void valueTreeParentChanged (juce::ValueTree&) override{};
    void valueTreeRedirected (juce::ValueTree&) override;
    bool isSuitableType (const juce::ValueTree& v) const override
    {
        return true;
    }


    ModulesInterface(juce::ValueTree &);
    virtual ~ModulesInterface();

    void paintBackground(juce::Graphics& g) override;
    void paintChildrenShadows(juce::Graphics& g) override { }
    void resized() override;
    void redoBackgroundImage();


    void setFocus() { grabKeyboardFocus(); }
    virtual void setEffectPositions() = 0;

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;

    void scrollBarMoved(ScrollBar* scroll_bar, double range_start) override;
    void setScrollBarRange();

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void effectsScrolled(int position) override {
        setScrollBarRange();
        scroll_bar_->setCurrentRange(position, viewport_.getHeight());

        for (Listener* listener : listeners_)
            listener->effectsMoved();
    }
    void mouseDown(const juce::MouseEvent&e);
    virtual PopupItems createPopupMenu() = 0;
    virtual void handlePopupResult(int result) = 0;
protected:
    std::vector<Listener*> listeners_;
    EffectsViewport viewport_;
    std::unique_ptr<ModulesContainer> container_;
    OpenGlImage background_;
    CriticalSection open_gl_critical_section_;

    std::unique_ptr<OpenGlScrollBar> scroll_bar_;
//
//    std::vector<std::unique_ptr<SynthSection>> modules;


    Factory<juce::AudioProcessor> factory;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulesInterface)
};
template<typename T>
ModulesInterface<T>::ModulesInterface(juce::ValueTree &v) : SynthSection("modules"), tracktion::engine::ValueTreeObjectList<T>(v) {
    container_ = std::make_unique<ModulesContainer>("container");

    addAndMakeVisible(viewport_);
    viewport_.setViewedComponent(container_.get());
    viewport_.addListener(this);
    viewport_.setScrollBarsShown(false, false, true, false);

    addSubSection(container_.get(), false);




    setOpaque(false);


    ////    setSkinOverride(Skin::kAllEffects);
}
template<typename T>
ModulesInterface<T>::~ModulesInterface() {
    //freeObjects();
}
template<typename T>
void ModulesInterface<T>::paintBackground(Graphics& g) {
    Colour background = findColour(Skin::kBackground, true);
    g.setColour(background);
    g.fillRect(getLocalBounds().withRight(getWidth() - findValue(Skin::kLargePadding) / 2));
    //paintChildBackground(g, effect_order_.get());

    redoBackgroundImage();
}
template<typename T>
void ModulesInterface<T>::redoBackgroundImage() {
    Colour background = findColour(Skin::kBackground, true);

    int height = std::max(container_->getHeight(), getHeight());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
    Image background_image = Image(Image::ARGB, container_->getWidth() * mult, height * mult, true);

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
    viewport_.setBounds(viewport_x, 0, viewport_width, getHeight());
    setEffectPositions();

    scroll_bar_->setBounds(getWidth() - large_padding + 1, 0, large_padding - 2, getHeight());
    scroll_bar_->setColor(findColour(Skin::kLightenScreen, true));

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
}



template<typename T>
void ModulesInterface<T>::initOpenGlComponents(OpenGlWrapper& open_gl) {
    background_.init(open_gl);
    SynthSection::initOpenGlComponents(open_gl);
}

template<typename T>
void ModulesInterface<T>::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
    ScopedLock lock(open_gl_critical_section_);
    //    Component* top_level = getTopLevelComponent();
    //    Rectangle<int> global_bounds = top_level->getLocalArea(this, getLocalBounds());
    //    double display_scale = Desktop::getInstance().getDisplays().getDisplayForRect(top_level->getScreenBounds())->scale;
    //    return 1;//
    // display_scale;// * (1.0f * global_bounds.getWidth()) / getWidth();
    OpenGlComponent::setViewPort(&viewport_, open_gl);

    float image_width = background_.getImageWidth(); //electrosynth::utils::nextPowerOfTwo(background_.getImageWidth());
    float image_height =background_.getImageHeight();  electrosynth::utils::nextPowerOfTwo(background_.getImageHeight());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
    float width_ratio = image_width / (container_->getWidth() * mult);
    float height_ratio = image_height / (viewport_.getHeight() * mult);
    float y_offset = (2.0f * viewport_.getViewPositionY()) / getHeight();

    background_.setTopLeft(-1.0f, 1.0f + y_offset);
    background_.setTopRight(-1.0 + 2.0f * width_ratio, 1.0f + y_offset);
    background_.setBottomLeft(-1.0f, 1.0f - 2.0f * height_ratio + y_offset);
    background_.setBottomRight(-1.0 + 2.0f * width_ratio, 1.0f - 2.0f * height_ratio + y_offset);
    background_.setColor(Colours::white);
    background_.drawImage(open_gl);
    SynthSection::renderOpenGlComponents(open_gl, animate);
}
template<typename T>
void ModulesInterface<T>::destroyOpenGlComponents(OpenGlWrapper& open_gl) {
    background_.destroy(open_gl);
    SynthSection::destroyOpenGlComponents(open_gl);
}
template<typename T>
void ModulesInterface<T>::scrollBarMoved(ScrollBar* scroll_bar, double range_start) {
    viewport_.setViewPosition(juce::Point<int>(0, range_start));
}
template<typename T>
void ModulesInterface<T>::setScrollBarRange() {
    scroll_bar_->setRangeLimits(0.0, container_->getHeight());
    scroll_bar_->setCurrentRange(scroll_bar_->getCurrentRangeStart(), viewport_.getHeight(), dontSendNotification);
    DBG("container height: " + String(container_->getHeight()));
    DBG("viewport height: " + String(viewport_.getHeight()));
    DBG("scrollbar range: " + String(scroll_bar_->getCurrentRangeStart()) );
}

template<typename T>
void ModulesInterface<T>::newObjectAdded (T*)
{
    resized();
}

template<typename T>
void ModulesInterface<T>::valueTreeRedirected (juce::ValueTree&)
{

}
#endif //ELECTROSYNTH_SOUND_GENERATOR_SECTION_H
