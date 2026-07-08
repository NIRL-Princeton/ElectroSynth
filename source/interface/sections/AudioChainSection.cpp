//
// Created by Davis Polito on 6/30/25.
//
// AudioChainSection.cpp is the UI layer above SoundModuleSection.cpp. It is responsible for the collection of distinct sound module sections.
// Each SoundModuleSection inside it is one seperate sound module. The main functions of this file is:
// 1) Owns the scrollable container holding all Sound Modules and manages scrolling for the whole list
// 2) Facilitates creation/destruction of entire sound modules.
// 3) Lays out each SoundModuleSection vertically in AudioChainSection::setEffectPositions()
// 4) Forwards listener events when modules are moved, added, or removed
// 5) Assigns top-level display numbers.


#include "AudioChainSection.h"
#include "synth_gui_interface.h"
#include "synth_base.h"
#include "about_section.h"
#include "modulation_manager.h"
#include "FullInterface.h"

AudioChainSection::AudioChainSection(ChainList<ProcessorBase> &chains, ModulationManager *m, juce::UndoManager& um) : SynthSection("chains"),
    chains_(chains), modulation_manager_(m), undo(um) {

    container_ = std::make_unique<ModulesListContainer>("container");

    addAndMakeVisible(viewport_);
    viewport_.setViewedComponent(container_.get());
    viewport_.addListener(this);
    viewport_.setInterceptsMouseClicks(false, true);
    addSubSection(container_.get(), false);
    setOpaque(true);
    chains_.addListener(this);

    scroll_bar_ = std::make_unique<OpenGlScrollBar>();
    addAndMakeVisible(scroll_bar_.get());
    addOpenGlComponent(scroll_bar_->getGlComponent());
    scroll_bar_->addListener(this);
    viewport_.setScrollBarPosition(true, false); //use this to determine viewport scroll type in effectsviewport
    viewport_.setScrollBarsShown(false, false, true, false);

    setSidewaysHeading(false);
    addListener(m);

    if (chains_.isEmpty()) {
        juce::ValueTree oscillator(IDs::SOUNDMODULE);
        oscillator.setProperty(IDs::type, "osc", nullptr);

        juce::ValueTree default_chain(IDs::CHAIN);
        default_chain.appendChild(oscillator, nullptr);
        chains_.appendChild(default_chain, nullptr);
    }
}

AudioChainSection::~AudioChainSection() {
    chains_.removeListener(this);
}

void AudioChainSection::paintBackground(juce::Graphics &g) {
    {
        static constexpr float kAudioChainBorderWidth = 20.0f;

        g.setColour(findColour(Skin::kBody, true));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), findValue(Skin::kBodyRounding));
        g.setColour(findColour(Skin::kBorder, true));

        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(kAudioChainBorderWidth * 0.5f),
                               findValue(Skin::kBodyRounding), kAudioChainBorderWidth);

        redoBackgroundImage();
    }
}

void AudioChainSection::redoBackgroundImage() {

    int height = std::max(container_->getHeight(), getHeight());
    int width = std::max(container_->getWidth(), getWidth());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;

    Image background_image = Image(Image::ARGB, width * mult, height * mult, true);

    Graphics background_graphics(background_image);
    background_graphics.addTransform(AffineTransform::scale(mult));
    container_->paintBackground(background_graphics);
    background_.setOwnImage(background_image);
}

void AudioChainSection::resized() {
    static constexpr float kEffectOrderWidthPercent = 0.2f;
    static constexpr int kScrollBarInset = 14;
    static constexpr int kScrollBarWidth = 7;
    ScopedLock lock(open_gl_critical_section_);

    int large_padding = findValue(Skin::kLargePadding);
    int shadow_width = getComponentShadowWidth();

    viewport_.setBounds(0, 0, getWidth(), getHeight());

    setEffectPositions();
    const int scroll_bar_height = std::max(0, static_cast<int>(getHeight() - getTitleWidth() - (large_padding + 2 * shadow_width)));
    scroll_bar_->setBounds(getWidth() - kScrollBarInset - kScrollBarWidth, getTitleWidth() + large_padding, kScrollBarWidth, scroll_bar_height);
    scroll_bar_->setColor(findColour(Skin::kWidgetPrimary1, true));
    scroll_bar_->setVisible(container_->getHeight() > viewport_.getHeight());

    SynthSection::resized();
}


void AudioChainSection::initOpenGlComponents(OpenGlWrapper &open_gl) {
    background_.init(open_gl);
    SynthSection::initOpenGlComponents(open_gl);
}


void AudioChainSection::renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) {
    ScopedLock lock(open_gl_critical_section_);

    OpenGlComponent::setViewPort(&viewport_, open_gl);

    float image_width = background_.getImageWidth(); //electrosynth::utils::nextPowerOfTwo(background_.getImageWidth());
    float image_height = background_.getImageHeight();
    // electrosynth::utils::nextPowerOfTwo(background_.getImageHeight());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;
    // getPixelMultiple();
    float width_ratio = image_width / (container_->getWidth() * mult);
    float height_ratio = image_height / (viewport_.getHeight() * mult);
    // DBG(viewport_.getViewPositionY());
    float y_offset = (2.0f * viewport_.getViewPositionY()) / getHeight();

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
    background_.setTopLeft(-1.0f, 1.0f + y_offset);
    background_.setTopRight(-1.0f + 2.0f * width_ratio, 1.0f + y_offset);
    background_.setBottomLeft(-1.0f, 1.0f - 2.0f * height_ratio + y_offset);
    background_.setBottomRight(-1.0f + 2.0f * width_ratio, 1.0f - 2.0f * height_ratio + y_offset);
    background_.setColor(Colours::white);
    background_.drawImage(open_gl);
    OpenGlComponent::setScissorBounds(this, viewport_.getBounds(), open_gl);
    SynthSection::renderOpenGlComponents(open_gl, animate);
    // DBG("TopLeft: (" + juce::String(-1.0f) + ", " + juce::String(1.0f + y_offset) + ")");
    // DBG("TopRight: (" + juce::String(-1.0f + 2.0f * width_ratio) + ", " + juce::String(1.0f + y_offset) + ")");
    // DBG("BottomLeft: (" + juce::String(-1.0f) + ", " + juce::String(1.0f - 2.0f * height_ratio + y_offset) + ")");
    // DBG("BottomRight: (" + juce::String(-1.0f + 2.0f * width_ratio) + ", " + juce::String(1.0f - 2.0f * height_ratio + y_offset) + ")");
}

void AudioChainSection::destroyOpenGlComponents(juce::OpenGLContext &open_gl) {
    background_.destroy(open_gl);
    SynthSection::destroyOpenGlComponents(open_gl);
}

void AudioChainSection::scrollBarMoved(ScrollBar *scroll_bar, double range_start) {
    viewport_.setViewPosition(juce::Point<int>(0, std::ceil(range_start)));
    // DBG(range_start);
}

void AudioChainSection::setScrollBarRange() {
    scroll_bar_->setRangeLimits(0.0, container_->getHeight());
    scroll_bar_->setCurrentRange(scroll_bar_->getCurrentRangeStart(), viewport_.getHeight(), dontSendNotification);
    //   DBG("container height: " + String(container_->getHeight()));
    //  DBG("viewport height: " + String(viewport_.getHeight()));
    // DBG("scrollbar range: " + String(scroll_bar_->getCurrentRangeStart()) );
}

void AudioChainSection::reset() {
    SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();
    if (_parent != nullptr)
        chains_.setValueTree(_parent->getSynth()->tree.getChildWithName(IDs::CHAINS));
    SynthSection::reset();
}

void AudioChainSection::setEffectPositions() {
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    int padding = getPadding();
    int large_padding = findValue(Skin::kLargePadding);
    int shadow_width = getComponentShadowWidth();
    int start_x = large_padding - shadow_width;
    int effect_width = getWidth() - start_x - large_padding;
    int knob_section_height = getKnobSectionHeight();
    int widget_margin = findValue(Skin::kWidgetMargin);
    int effect_height =  knob_section_height + getTitleWidth()*4 - widget_margin;
    int y = 0;

    juce::Point<int> position = viewport_.getViewPosition();
    // DBG("position viewport: x: " + juce::String(position.getX()) + "y: " + juce::String(position.getY()));
    //DBG("shadwo width: " + String(shadow_width));

    int sound_module_index = 1;
    for (auto &section: sound_module_sections) {
        section->setSoundModuleIndex(sound_module_index++);
        if (section->isExpanded()) {
            const int sectionheight = section->getExpandedHeight();
            section->setBounds(0, y, effect_width, sectionheight );
            y += (sectionheight + padding);
        } else {
            const int collapsed_height = section->getCollapsedHeight();
            section->setBounds(0, y, effect_width, collapsed_height);
            y += (collapsed_height + padding);
        }
    }
    container_->setBounds(0, 0, viewport_.getWidth(), y - padding + effect_height * 2 );
    viewport_.setViewPosition(position);

    for (Listener *listener: listeners_)
        listener->effectsMoved();

    //DBG("container Height " + String(container_->getHeight()));
    //DBG("viewport Height " + String(viewport_.getWidth()));
    container_->setScrollWheelEnabled(container_->getHeight() <= viewport_.getHeight());
    setScrollBarRange();
    repaintBackground();
}
#include "FullInterface.h"

void AudioChainSection::removeChain(ModuleList<ProcessorBase> *moduleToRemove) {

    auto it = [&]() {
        juce::ScopedLock lock(this->open_gl_critical_section_);
        return std::partition(sound_module_sections.begin(), sound_module_sections.end(),
                              [moduleToRemove](auto& section) {
                                  return section->state != moduleToRemove->state;
                              });
    }();

    it->get()->setVisible(false);

    auto *_parent = findParentComponentOfClass<SynthGuiInterface>();
    _parent->getOpenGlWrapper()->context.executeOnGLThread([this, it](juce::OpenGLContext &openGLContext) {
        auto a = it->get();
        a->destroyOpenGlComponents(openGLContext);
        this->container_->removeSubSection(a);
        DBG("deleteonopengl");
        },true);

    {
        juce::ScopedLock lock(open_gl_critical_section_);
        sound_module_sections.erase(it);
        DBG("deletesection");
    }

    for(auto listener : listeners_) {listener->removed();}

    DBG("finishcrit");
    resized();
}

void AudioChainSection::chainAdded(ModuleList<ProcessorBase> *module_list) {

    auto sound_interface = std::make_unique<SoundModuleSection>(modulation_manager_, *module_list,module_list->state, undo);
    auto* rawPtr = sound_interface.get();

    sound_interface->onExpandChanged = [this,rawPtr]() {
        if (rawPtr->isExpanded()) {
            rawPtr->setSize (rawPtr->getWidth(), rawPtr->height);
        }
        resized(); //sound_interface->redoBackgroundImage();
        // rawPtr->setSize(getWidth(), rawPtr->height);
        auto full = findParentComponentOfClass<FullInterface>();
        full->redoBackground();
    };

    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    if (interface != nullptr) {
        interface->getOpenGlWrapper()->context.executeOnGLThread(
            [this, a = sound_interface.get()](juce::OpenGLContext &openGLContext) {
                auto interface = this->findParentComponentOfClass<SynthGuiInterface>();
                if (interface != nullptr) {
                    a->initOpenGlComponents(*interface->getOpenGlWrapper());
                }
            }, true);
    } {
        juce::ScopedLock lock(open_gl_critical_section_);
        container_->addSubSection(sound_interface.get());
    }
    sound_interface->addListener(this);
    sound_module_sections.emplace_back(std::move(sound_interface));

    resized();

    for (auto* listener : listeners_)
        listener->added();
}

void AudioChainSection::chainChanged() {
}

PopupItems AudioChainSection::createPopupMenu() {
    PopupItems options;
    options.addItem(1, "add oscillator");
    options.addItem(2, "add string");
    return options;
}

void AudioChainSection::handlePopupResult(int result) {
    if (result == 1) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "osc", nullptr);
        juce::ValueTree v(IDs::CHAIN);
        undo.beginNewTransaction();
        v.appendChild(t, &undo);
        chains_.appendChild(v, &undo);
    }
    if (result == 2) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "string", nullptr);
        juce::ValueTree v(IDs::CHAIN);
        undo.beginNewTransaction();
        v.appendChild(t, &undo);
        chains_.appendChild(v, &undo);
    }
}

std::map<std::string, SynthSlider *> AudioChainSection::getAllSliders() {
    std::map<std::string, SynthSlider *> sliders;
    DBG("getAllSliders");
    for (auto &obj: sound_module_sections) {
        auto section_sliders = obj->getAllSliders();
        sliders.insert(section_sliders.begin(), section_sliders.end());
    }
    return sliders;
}
