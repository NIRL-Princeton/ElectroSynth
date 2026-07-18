//
// Created by Davis Polito on 11/19/24.
//


#include "ModulationModuleSection.h"
#include "ModulationSection.h"
#include "Modulators/EnvModuleProcessor.h"
#include "Modulators/LFOModuleProcessor.h"
#include "synth_gui_interface.h"
#include "modulation_manager.h"
#include "synth_base.h"

namespace electrosynth {
    class SoundEngine;
}

ModulationModuleSection::ModulationModuleSection(ModulationManager *modulation_manager, ModuleList<ModulatorBase>& module_list,
    juce::UndoManager& um) : ModulesInterface(module_list), modulation_manager(modulation_manager), undo(um)
{
    setName("Modulation");
    toggle_button_->setVisible(false);

    header_body_ = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, "modulation_header");
    header_body_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(header_body_, true);

    header_title_ = std::make_shared<PlainTextComponent>("modulation_title", getName());
    header_title_->setFontType(PlainTextComponent::kLight);
    header_title_->setJustification(juce::Justification::centred);
    header_title_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(header_title_);

    // Modulation tabs
    for (int i = 0; i < kMaxTabs; ++i) {
        tab_buttons_[i] = std::make_unique<OpenGlToggleButton>("modulation_tab_ " + juce::String(i));
        tab_buttons_[i]->setClickingTogglesState(false);
        tab_buttons_[i]->setJustification(juce::Justification::centredLeft);
        addButton(tab_buttons_[i].get(), true);

        tab_borders_[i] = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleBorderFragment, "modulation_tab_border_" + juce::String(i));
        tab_borders_[i]->setInterceptsMouseClicks(false, false);
        tab_borders_[i]->setThickness(1.3f, true);
        addOpenGlComponent(tab_borders_[i]);

        selected_tab_bottoms_[i] = std::make_shared<OpenGlQuad>(
            Shaders::kColorFragment, "selected_modulation_tab_bottom_" + juce::String(i));
        selected_tab_lefts_[i] = std::make_shared<OpenGlQuad>(
            Shaders::kColorFragment, "selected_modulation_tab_left_" + juce::String(i));
        selected_tab_rights_[i] = std::make_shared<OpenGlQuad>(
            Shaders::kColorFragment, "selected_modulation_tab_right_" + juce::String(i));
        selected_tab_line_masks_[i] = std::make_shared<OpenGlQuad>(
            Shaders::kColorFragment, "selected_modulation_tab_line_mask_" + juce::String(i));

        selected_tab_line_masks_[i]->setInterceptsMouseClicks(false, false);
        addOpenGlComponent(selected_tab_line_masks_[i]);

        for (auto edge : { selected_tab_bottoms_[i], selected_tab_lefts_[i], selected_tab_rights_[i] }) {
            edge->setInterceptsMouseClicks(false, false);
            edge->setColor(juce::Colours::white);
            addOpenGlComponent(edge);
        }
    }

    // Add modulator plus sign
    add_mod_button_background_ = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleFragment, "modulation_button_background_");
    add_mod_button_background_->setInterceptsMouseClicks(false, false);
    add_mod_button_background_->setRounding(5.0f);
    addOpenGlComponent (add_mod_button_background_);

    add_modulator_button_ = std::make_unique<OpenGlShapeButton>("Add Modulator");
    addAndMakeVisible (add_modulator_button_.get());
    addOpenGlComponent (add_modulator_button_->getGlComponent());
    add_modulator_button_->addListener (this);
    add_modulator_button_->setShape(Paths::plus (150));
    add_modulator_button_->addMouseListener (this, false);


    scroll_bar_ = std::make_unique<OpenGlScrollBar>(false);
//    scroll_bar_->setShrinkLeft(true)
    addAndMakeVisible(scroll_bar_.get());
    addOpenGlComponent(scroll_bar_->getGlComponent());
    scroll_bar_->addListener(this);
    setSkinOverride(Skin::kModulation);
    //Skin default_skin;
    //setSkinValues(default_skin,false);
    viewport_.setScrollBarPosition(false,true);
    viewport_.setScrollBarsShown(false, false, false, true);

    addListener(modulation_manager);

    if (list.state.getNumChildren() == 0) {
        juce::ValueTree default_envelope(IDs::MODULATOR);
        default_envelope.setProperty(IDs::type, "env", nullptr);
        list.appendChild(default_envelope, nullptr);
    }

    //setInterceptsMouseClicks(false, true);
}

ModulationModuleSection::~ModulationModuleSection() {
    module_sections.clear();
}

void ModulationModuleSection::resized() {

    static constexpr float kEffectOrderWidthPercent = 0.2f;
    static constexpr int kAddButtonSize = 34;
    static constexpr int kAddButtonGap = 6;

    ScopedLock lock(open_gl_critical_section_);

    const int title_width = static_cast<int>(getTitleWidth());
    const int tab_strip_y = getHeight() - kTabStripHeight;

    // set horizontal bounds of modulation tabs
    const int tab_width = (getWidth()- kMaxTabs) / kMaxTabs;
    for (int i = 0; i < kMaxTabs; ++i) {
        const int left = i * tab_width;
        const int right = (i + 1) * tab_width;
        const juce::Rectangle<int> outline_bounds(
            left + 4, tab_strip_y + 4, std::max(0, right - left - 8), kTabStripHeight - 8);
        tab_borders_[i]->setBounds(outline_bounds);

        // Shift the text two pixels right without moving the outline.
        tab_buttons_[i]->setBounds(outline_bounds.getX() + 4, outline_bounds.getY(),
                                   std::max(0, outline_bounds.getWidth() - 4), outline_bounds.getHeight());

        static constexpr int kSelectedTabLineThickness = 2;
        static constexpr int kSelectedTabSideExtension = 4;
        selected_tab_bottoms_[i]->setBounds(
            outline_bounds.getX(), outline_bounds.getBottom() - kSelectedTabLineThickness,
            outline_bounds.getWidth(), kSelectedTabLineThickness);
        selected_tab_lefts_[i]->setBounds(
            outline_bounds.getX(), outline_bounds.getY() - kSelectedTabSideExtension,
            kSelectedTabLineThickness,
            outline_bounds.getHeight() + kSelectedTabSideExtension);
        selected_tab_rights_[i]->setBounds(
            outline_bounds.getRight() - kSelectedTabLineThickness,
            outline_bounds.getY() - kSelectedTabSideExtension,
            kSelectedTabLineThickness,
            outline_bounds.getHeight() + kSelectedTabSideExtension);
        selected_tab_line_masks_[i]->setBounds(
            outline_bounds.getX() + kSelectedTabLineThickness,
            outline_bounds.getY() - kSelectedTabSideExtension - 1,
            std::max(0, outline_bounds.getWidth() - 2 * kSelectedTabLineThickness),
            kSelectedTabSideExtension + 2);
    }

    const int tabCount = static_cast<int>(module_sections.size());
    if (tabCount < kMaxTabs) {
        const int lastTabIndex = std::max(0, tabCount - 1);
        const auto lastTabBounds = tabCount > 0 ? tab_buttons_[lastTabIndex]->getBounds()
        : juce::Rectangle<int>(0, getHeight()-kTabStripHeight + 4, 0, kTabStripHeight - 8);

        const int x = lastTabBounds.getRight() + kAddButtonGap;
        const int y = lastTabBounds.getCentreY() - kAddButtonSize/2;

        add_modulator_button_->setVisible(true);
        add_modulator_button_->setBounds(x, y, kAddButtonSize, kAddButtonSize);
        add_modulator_button_->setColour(Skin::kIconButtonOff, findColour(Skin::kIconButtonOff, true));
        add_modulator_button_->setColour(Skin::kIconButtonOffHover, findColour(Skin::kIconButtonOffHover, true));
        add_modulator_button_->setColour(Skin::kIconButtonOffPressed, findColour(Skin::kIconButtonOffPressed, true));


        add_mod_button_background_->setVisible(true);
        add_mod_button_background_->setBounds(add_modulator_button_->getBounds().reduced(5.f));
        add_mod_button_background_->setColor(findColour(Skin::kBorder, true));
    }
    else {
        add_modulator_button_->setVisible(false);
        add_mod_button_background_->setVisible(false);
    }


    viewport_.setBounds(0, title_width, getWidth(),
                        std::max(0, getHeight() - title_width - kTabStripHeight));
    setEffectPositions();

    scroll_bar_->setBounds(0, 0, 0, 0);
    scroll_bar_->setVisible(false);

    SynthSection::resized();

    header_body_->setBounds(0, 0, getWidth(), title_width);
    header_body_->setColor(findColour(Skin::kBodyHeading, true));
    header_title_->setBounds(0, 0, getWidth(), title_width);
    header_title_->setText(getName());
    header_title_->setTextSize(size_ratio_ * 14.0f);
    header_title_->setColor(findColour(Skin::kHeadingText, true));
    updateTabs();
}

void ModulationModuleSection::paintBackground(juce::Graphics& g) {
    g.setColour(findColour(Skin::kBody, true));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), findValue(Skin::kBodyRounding));
    paintBody(g);

    g.setColour(findColour(Skin::kBorder, true));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), findValue(Skin::kBodyRounding), 1.0f);
    paintBorder(g);

    redoBackgroundImage();
}

void ModulationModuleSection::mouseEnter(const MouseEvent& event) {
    if (event.eventComponent == add_modulator_button_.get()) {
        showPopupDisplay(
            add_modulator_button_.get(),
            "Click to add modulator",
            juce::BubbleComponent::right,
            true);
    }
}

 void ModulationModuleSection::mouseExit(const MouseEvent& event) {
    if (event.eventComponent == add_modulator_button_.get())
        hidePopupDisplay(true);
}

void ModulationModuleSection::handlePopupResult(int result) {

    //std::vector<vital::ModulationConnection*> connections = getConnections();
    if (result == 1 )
    {
        juce::ValueTree t(IDs::MODULATOR);
        t.setProperty(IDs::type, "env", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t,&undo);
    }
    else if (result == 2 )
    {
        juce::ValueTree t(IDs::MODULATOR);
        t.setProperty(IDs::type, "lfo", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t,&undo);
    }
    else if (result == 3 )
    {
        juce::ValueTree t(IDs::MODULATOR);
        t.setProperty(IDs::type, "simpNos", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t,&undo);
    }
    else if (result == 4 )
    {
        juce::ValueTree t(IDs::MODULATOR);
        t.setProperty(IDs::type, "perlNos", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t,&undo);
    }

}


void ModulationModuleSection::setEffectPositions() {
    if (getWidth() <= 0 || getHeight() <= 0) return;

    const int effect_height = viewport_.getHeight();

    if (!module_sections.empty()) selected_tab_ = juce::jlimit(0, static_cast<int>(module_sections.size()) - 1, selected_tab_);

    for (int i = 0; i < static_cast<int>(module_sections.size()); ++i) {
        const bool selected = (i == selected_tab_);
        module_sections[i]->setVisible(selected);
        if (selected) module_sections[i]->setBounds(0, 0, viewport_.getWidth(), effect_height);
    }

    container_->setBounds(0, 0, viewport_.getWidth(), viewport_.getHeight());
    viewport_.setViewPosition(0, 0);

    for (Listener* listener : listeners_)
        listener->effectsMoved();

    container_->setScrollWheelEnabled(true);
    setScrollBarRange();
    repaintBackground();
}

void ModulationModuleSection::buttonClicked(juce::Button* button) {
    if (button == add_modulator_button_.get()) {
        hidePopupDisplay(true);
        showPopupSelector (add_modulator_button_.get(), add_modulator_button_->getLocalBounds().getCentre(),
            createPopupMenu(), [this] (int selection) {handlePopupResult (selection);});
    }


    for (int i = 0; i < kMaxTabs; ++i) {
        if (button == tab_buttons_[i].get() && i < static_cast<int>(module_sections.size())) {
            selected_tab_ = i;
            setEffectPositions();
            updateTabs();
            return;
        }
    }
    ModulesInterface<ModulatorBase>::buttonClicked(button);
}

void ModulationModuleSection::updateTabs() {
    int env_number = 0;
    int lfo_number = 0;

    for (int i = 0; i < kMaxTabs; ++i) {
        const bool occupied = i < static_cast<int>(module_sections.size());
        tab_buttons_[i]->setVisible(occupied);
        if (!occupied) {
            tab_borders_[i]->setVisible(false);
            selected_tab_bottoms_[i]->setVisible(false);
            selected_tab_lefts_[i]->setVisible(false);
            selected_tab_rights_[i]->setVisible(false);
            selected_tab_line_masks_[i]->setVisible(false);
            continue;
        }

        const bool is_envelope = module_sections[i]->getModulatorType().equalsIgnoreCase("env");
        const bool selected = i == selected_tab_;
        const auto accent = is_envelope
                                ? ShaderColors::kEnvelopeTextColor
                                : ShaderColors::kLfoTextColor;
        const int number = is_envelope ? ++env_number : ++lfo_number;
        const auto label = (is_envelope ? "Env " : "LFO ") + juce::String(number);
        tab_buttons_[i]->setText("  " + label);
        tab_buttons_[i]->setToggleState(selected, juce::dontSendNotification);
        tab_buttons_[i]->setColour(Skin::kBody, findColour(Skin::kBody, true));
        tab_buttons_[i]->setColour(Skin::kTextComponentBackground, selected ? juce::Colours::transparentBlack : juce::Colours::black);
        tab_buttons_[i]->setColour(Skin::kIconButtonOn, accent);
        tab_buttons_[i]->setColour(Skin::kIconButtonOnPressed, accent);
        tab_buttons_[i]->setColour(Skin::kIconButtonOnHover, accent.brighter(0.15f));
        tab_buttons_[i]->setColour(Skin::kIconButtonOff, accent);
        tab_buttons_[i]->setColour(Skin::kIconButtonOffPressed, accent);
        tab_buttons_[i]->setColour(Skin::kIconButtonOffHover, accent.brighter(0.15f));
        tab_buttons_[i]->getGlComponent()->setColors();

        if (auto* mod_button = module_sections[i]->getModulationButton()) {
            static constexpr int kModButtonSize = 25;
            static constexpr int kTextLeftPadding = 12;
            static constexpr int kTextIconGap = 35;

            const auto tab_bounds = tab_buttons_[i]->getBounds();
            const int text_width = juce::GlyphArrangement::getStringWidth(getLabelFont(), label);
            const int max_icon_x = tab_bounds.getRight() - kModButtonSize;
            const int wanted_icon_x = tab_bounds.getX() + kTextLeftPadding + text_width + kTextIconGap;
            const int icon_x = max_icon_x >= tab_bounds.getX()
                                   ? juce::jlimit(tab_bounds.getX(), max_icon_x, wanted_icon_x)
                                   : tab_bounds.getX();
            mod_button->setSourceColor(accent);
            mod_button->setVisible(occupied);
            mod_button->setBounds(icon_x,
                                  tab_bounds.getCentreY() - kModButtonSize / 2,
                                  kModButtonSize,
                                  kModButtonSize);
            mod_button->toFront(false);
            mod_button->setDisplayLabel(label);
        }

        tab_borders_[i]->setVisible(!selected);
        tab_borders_[i]->setColor(accent);
        selected_tab_bottoms_[i]->setVisible(selected);
        selected_tab_bottoms_[i]->setColor(accent);
        selected_tab_lefts_[i]->setVisible(selected);
        selected_tab_lefts_[i]->setColor(accent);
        selected_tab_rights_[i]->setVisible(selected);
        selected_tab_rights_[i]->setColor(accent);
        selected_tab_line_masks_[i]->setColor(findColour(Skin::kBody, true));
        selected_tab_line_masks_[i]->setVisible(selected);
    }


}
PopupItems ModulationModuleSection::createPopupMenu() {
    PopupItems options;
    options.addItem(1, "add Env" );
    options.addItem(2, "add LFO" );
    options.addItem(3, "add White Noise");
    options.addItem(4, "add Perlin Noise");

    return options;
}
void ModulationModuleSection::scrollBarMoved(ScrollBar* scroll_bar, double range_start) {
    viewport_.setViewPosition(juce::Point<int>(range_start,0));
    DBG("rangestart "  + juce::String(range_start));
}

void ModulationModuleSection::setScrollBarRange() {
    scroll_bar_->setRangeLimits(0.0, container_->getWidth() );
    scroll_bar_->setCurrentRange(scroll_bar_->getCurrentRangeStart(), viewport_.getWidth(), dontSendNotification);
    // repaintBackground();
    //DBG("container width " + String(container_->getWidth()));
   // DBG("viewport wdith " + String(viewport_.getWidth()));

   // DBG("scrollbar range: " + String(scroll_bar_->getCurrentRangeStart()) );
}
void ModulationModuleSection::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
    ScopedLock lock(open_gl_critical_section_);
    //    Component* top_level = getTopLevelComponent();
    //    Rectangle<int> global_bounds = top_level->getLocalArea(this, getLocalBounds());
    //    double display_scale = Desktop::getInstance().getDisplays().getDisplayForRect(top_level->getScreenBounds())->scale;
    //    return 1;//
    // display_scale;// * (1.0f * global_bounds.getWidth()) / getWidth();
    OpenGlComponent::setViewPort(&viewport_, open_gl);

    float image_width = background_.getImageWidth(); //electrosynth::utils::nextPowerOfTwo(background_.getImageWidth());
    float image_height =background_.getImageHeight(); // electrosynth::utils::nextPowerOfTwo(background_.getImageHeight());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
    float width_ratio = image_width / (viewport_.getWidth() * mult);
    float height_ratio = image_height / (container_->getHeight() * mult);
    float x_offset = (-2.0f * viewport_.getViewPositionX()) / getWidth();

    background_.setTopLeft(-1.0f  + x_offset, 1.0f);
    background_.setTopRight(-1.0 + 2.0f * width_ratio +  x_offset, 1.0f);
    background_.setBottomLeft(-1.0f  + x_offset, 1.0f - 2.0f * height_ratio);
    background_.setBottomRight(-1.0 + 2.0f * width_ratio + x_offset, 1.0f - 2.0f * height_ratio);

    background_.setColor(Colours::white);
    background_.drawImage(open_gl);

//    DBG("x_offset" + juce::String(x_offset));
    SynthSection::renderOpenGlComponents(open_gl, animate);
}

void ModulationModuleSection::redoBackgroundImage() {
    Colour background = findColour(Skin::kBackground, true);

    int width = std::max(container_->getWidth(), getWidth());
    auto mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
    Image background_image = Image(Image::ARGB, width * mult,  container_->getHeight() * mult, true);

    Graphics background_graphics(background_image);
    background_graphics.addTransform(AffineTransform::scale(mult));
    background_graphics.fillAll(background);
    container_->paintBackground(background_graphics);
    background_.setOwnImage(background_image);
}
std::map<std::string, ModulationButton*> ModulationModuleSection::getAllModulationButtons() {
    //test_->getAllSliders();
    return container_->getAllModulationButtons();
}

void ModulationModuleSection::moduleAdded(ModulatorBase *newModule) {
    auto module_section = std::make_unique<ModulationSection>( newModule->state, std::move((newModule->createEditor())), undo);
    const bool is_lfo = module_section->getModulatorType().equalsIgnoreCase("lfo");
    module_section->setAreaSkinOverride(is_lfo ? Skin::kLfo : Skin::kEnvelope);

    {
        juce::ScopedLock lock(open_gl_critical_section_);
        container_->addSubSection(module_section.get());
    }
    module_section->applySkinFromTopLevel();

    module_section->setInterceptsMouseClicks(false,true);
    parentHierarchyChanged();
    module_sections.emplace_back(std::move(module_section));

    auto mod_button = module_sections.back()->getModulationButtonPtr();

    addOpenGlComponent(std::static_pointer_cast<OpenGlImageComponent>(mod_button));
    selected_tab_ = static_cast<int>(module_sections.size()) - 1;
    updateTabs();
    for(auto listener : listeners_) {
        listener->added();
    }

    resized();
    if (mod_button != nullptr)
        mod_button->redrawImage(true);
}

void ModulationModuleSection::moduleListChanged() {

}

void ModulationModuleSection::removeModule(ModulatorBase *newModule) {
    decltype(module_sections)::iterator it;
    {
        juce::ScopedLock(this->open_gl_critical_section_);
        it = std::find_if(module_sections.begin(), module_sections.end(),
                            [newModule](auto& section) {
                                return section->state == newModule->state;
                            });
    }
    //leaving this here as its another way to accomplish this task
    // auto it = [&]() {
    //     juce::ScopedLock lock(this->open_gl_critical_section_);
    //     return std::remove_if(module_sections.begin(), module_sections.end(),
    //                           [newModule](auto& section) {
    //                               return section->state == newModule->state;
    //                           });
    // }();


    if (it != module_sections.end()) {
        auto* section = it->get();
        section->setVisible(false);
        auto modulation_button = section->getModulationButtonPtr();
        if ((juce::OpenGLContext::getCurrentContext() == nullptr)) {

            auto *_parent = findParentComponentOfClass<SynthGuiInterface>();
            if (_parent != nullptr) {
                _parent->getOpenGlWrapper()->context.executeOnGLThread([this, section, modulation_button](juce::OpenGLContext &openGLContext) {


                    if (modulation_button != nullptr)
                        this->destroyOpenGlComponent(*modulation_button, openGLContext);
                    section->destroyOpenGlComponents(openGLContext);
                    this->container_->removeSubSection(section);

                    },true);
            }

        }
        else {
            auto& openGLContext = *juce::OpenGLContext::getCurrentContext();
            if (modulation_button != nullptr)
                destroyOpenGlComponent(*modulation_button, openGLContext);
            section->destroyOpenGlComponents(openGLContext);
            container_->removeSubSection(section);
        }

        module_sections.erase(it);
        selected_tab_ = juce::jlimit(0, std::max(0, static_cast<int>(module_sections.size()) - 1), selected_tab_);
        updateTabs();

        for(auto listener : listeners_)
        {
            listener->removed();
        }
        resized();
    }
}
