//
// Created by Davis Polito on 10/22/24.
//

// ModuleSection.cpp is the wrapper for one entire module (one oscillator, string, filter, etc.). It owns the module's actual
// editor view (ParametersView) and gives it a header, border/background, exit button, draft behavior, and a height.

#include "ModuleSection.h"
#include "SoundModuleSection.h"
#include "ParameterView/FxModuleTemplateView.h"

namespace {
    constexpr int kExitButtonSize = 25;
    constexpr int kExitButtonRightOffset = 50;
}

ModuleSection::ModuleSection(const juce::ValueTree &v, electrosynth::audio::NodeDescriptor node_descriptor, std::unique_ptr<SynthSection> editor,
    juce::UndoManager& um, AudioRoutingManager* arm) : SynthSection(editor->getName()), audioNodeDescriptor_ (std::move(node_descriptor)),
    state(v), _view(std::move(editor)), undo(um), audio_routing_manager_ (arm) {

    // The module's body fill is normally baked into the owning lane's scroll image and
    // does not follow a live-moving wrapper. While dragged, this quad supplies an opaque
    // traveling body. Created first so it renders below the title text, and non-always-
    // on-top so it stays below the always-on-top editor content.
    body_fill_ = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleFragment, "module_drag_body");
    body_fill_->setInterceptsMouseClicks(false, false);
    body_fill_->setAlpha(0.0f, true);
    addOpenGlComponent(body_fill_);

    setComponentID(_view->getName());
    addSubSection(_view.get());
    _view->setAlwaysOnTop(true);

    title_text_ = std::make_shared<PlainTextComponent>("module_title", getName());
    title_text_->setFontType(PlainTextComponent::kRegular);
    title_text_->setJustification(juce::Justification::centred);
    addOpenGlComponent(title_text_);

    bottom_separator_ = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, "module_bottom_separator");
    bottom_separator_->setAlwaysOnTop(true);
    bottom_separator_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(bottom_separator_);

    exit_button_ = std::make_unique<OpenGlShapeButton>("Exit");
    addAndMakeVisible(exit_button_.get());
    addOpenGlComponent(exit_button_->getGlComponent());
    exit_button_->addListener(this);
    exit_button_->setShape(Paths::exitX());
    exit_button_->setAlwaysOnTop(true);

    // Drag-reorder overlays. Always-on-top own GL components render after the
    // always-on-top editor sub-section, so these draw above the module's sliders.
    // The tint darkens non-dragged modules and adds a slight white sheen to the
    // dragged one; setDragVisual picks the color per state.
    tint_overlay_ = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, "module_drag_tint");
    tint_overlay_->setAlwaysOnTop(true);
    tint_overlay_->setInterceptsMouseClicks(false, false);
    tint_overlay_->setColor(juce::Colours::black);
    tint_overlay_->setAlpha(0.0f, true);
    addOpenGlComponent(tint_overlay_);

    highlight_border_ = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleBorderFragment,
                                                     "module_drag_highlight");
    highlight_border_->setAlwaysOnTop(true);
    highlight_border_->setInterceptsMouseClicks(false, false);
    highlight_border_->setAlpha(0.0f, true);
    addOpenGlComponent(highlight_border_);

    if (audioNodeDescriptor_.hasOutput) { // if this module supports outputs...
        electrosynth::audio::AudioPortAddress address { // give it an output audio port address
            getAudioNodeId(),
            audioNodeDescriptor_.outputPortId,
            electrosynth::audio::PortDirection::Output,
            audioNodeDescriptor_.domain
        };
        output_port_ = std::make_shared<AudioPortComponent>( // make an output arrow belonging to this output port
            "audio_output",
            std::move(address));
        addOpenGlComponent(output_port_);

        output_connection_slots_ = std::make_unique<AudioConnectionSlots>(*output_port_);
        addSubSection(output_connection_slots_.get());
        output_connection_slots_->setDestinations({});
    }

    if (audioNodeDescriptor_.hasInput) { // if this module supports inputs...
        electrosynth::audio::AudioPortAddress address { // give it an output audio port address
            getAudioNodeId(),
            audioNodeDescriptor_.inputPortId,
            electrosynth::audio::PortDirection::Input,
            audioNodeDescriptor_.domain
        };
        input_port_ = std::make_shared<AudioPortComponent>( // make an output arrow belonging to this output port
            "audio_input",
            std::move(address));
        addOpenGlComponent(input_port_);

        input_connection_slots_ = std::make_unique<AudioConnectionSlots>(*input_port_);
        addSubSection(input_connection_slots_.get());
        input_connection_slots_->setDestinations({});
    }

    if (audio_routing_manager_ != nullptr) {
        if (output_port_ != nullptr)
            audio_routing_manager_->registerPort(*output_port_);

        if (input_port_ != nullptr)
            audio_routing_manager_->registerPort(*input_port_);
    }
}

ModuleSection::~ModuleSection() {
    if (audio_routing_manager_ == nullptr)
        return;

    if (output_port_) audio_routing_manager_->unregisterPort(*output_port_);
    if (input_port_) audio_routing_manager_->unregisterPort(*input_port_);
}

void ModuleSection::setAreaSkinOverride(Skin::SectionOverride skin_override) {
    setSkinOverride(skin_override);
    if (_view != nullptr)
        _view->setSkinOverride(skin_override);
}

int ModuleSection::getPreferredHeight() const {
    return (_view != nullptr ? _view->getPreferredHeight() : 0) + kHeaderHeight;
}

int ModuleSection::refreshHeight() {
    height = getPreferredHeight();
    return height;
}

void ModuleSection::resized() {
    static constexpr int kAudioPortPanelWidth = 5;
    static constexpr int kAudioPortSize = 24;
    static constexpr int kAudioPortY = 5;
    static constexpr int kWidthOffset = 23;
    static constexpr int kConnectionSlotSpacing = 2;

    auto local = getLocalBounds();
    local.removeFromTop(kHeaderHeight);
    local.removeFromBottom(kContentBottomPadding);
    _view->setBounds(local);
    SynthSection::resized();

    title_text_->setBounds(0, 0, getWidth(), kHeaderHeight);
    title_text_->setText(getName());
    title_text_->setTextSize(static_cast<float>(kHeaderHeight) * 0.4f);
    title_text_->setColor(findColour(Skin::kHeadingText, true));


    int exit_x = getLocalBounds().getRight() - kExitButtonRightOffset;
    if (auto* fx_view = dynamic_cast<electrosynth::FxModuleTemplateView*>(_view.get())) {
        // FX modules: align the delete button's right edge with the right edge of the
        // PostGain knob's tick-arc box (last knob of the last row).
        const int postgain_right = fx_view->getPostGainRightEdge();
        if (postgain_right > 0)
            exit_x = juce::jlimit(0, std::max(0, getWidth() - kExitButtonSize),
                                  fx_view->getX() + postgain_right - kExitButtonSize);
    }
    else if (auto* sound_module = findParentComponentOfClass<SoundModuleSection>()) {
        const auto module_bounds_in_sound_module = sound_module->getLocalArea(this, getLocalBounds());
        const int sound_module_exit_x = sound_module->getWidth() - kExitButtonRightOffset;
        exit_x = juce::jlimit(0, std::max(0, getWidth() - kExitButtonSize),
                              sound_module_exit_x - module_bounds_in_sound_module.getX());
    }
    exit_button_->setBounds(exit_x, (kHeaderHeight - kExitButtonSize) / 2, kExitButtonSize, kExitButtonSize);

    if (output_port_) {
        output_port_->setBounds(getWidth() - kAudioPortPanelWidth - kWidthOffset, getHeight() - kAudioPortSize - kAudioPortY,
            kAudioPortSize, kAudioPortSize);
        output_port_->setColor(findColour(Skin::kWidgetPrimary1, true));
        output_port_->resized();

        if (output_connection_slots_) {
            output_connection_slots_->setBounds(
                output_port_->getX()
                    - kConnectionSlotSpacing
                    - AudioConnectionSlots::kPreferredWidth,
                output_port_->getY(),
                AudioConnectionSlots::kPreferredWidth,
                output_port_->getHeight());
        }
    }
    if (input_port_) {
        input_port_->setBounds(kAudioPortPanelWidth, getHeight() - kAudioPortSize - kAudioPortY,
            kAudioPortSize, kAudioPortSize);
        input_port_->setColor(findColour(Skin::kWidgetPrimary1, true));
        input_port_->resized();

        if (input_connection_slots_) {
            input_connection_slots_->setBounds(
                input_port_->getRight() + kConnectionSlotSpacing,
                input_port_->getY(),
                AudioConnectionSlots::kPreferredWidth,
                input_port_->getHeight());
        }
    }

    bottom_separator_->setBounds(0, std::max(0, getHeight() - 1), getWidth(), 2);
    bottom_separator_->setColor(findColour(Skin::kWidgetAccent1, true));
    bottom_separator_->setVisible(true); // setVisible(draw_bottom_separator_);

    body_fill_->setBounds(getLocalBounds());
    body_fill_->setRounding(findValue(Skin::kBodyRounding));
    tint_overlay_->setBounds(getLocalBounds());
    highlight_border_->setBounds(getLocalBounds());
    // Half-unit rounding floor keeps the zero-radius border shader's interior transparent
    // (same correction as the FX lane border overlays).
    highlight_border_->setRounding(std::max(0.5f, findValue(Skin::kBodyRounding)));
    setDragVisual(drag_visual_);
}

void ModuleSection::setDragVisual(DragVisual visual) {
    drag_visual_ = visual;
    if (tint_overlay_ == nullptr || highlight_border_ == nullptr || body_fill_ == nullptr)
        return;

    body_fill_->setColor(findColour(Skin::kBody, true));
    body_fill_->setAlpha(visual == DragVisual::kDragged ? 1.0f : 0.0f);

    // During a drag session the boundary/insertion accents replace the grey
    // separator, so hide it rather than layering red over grey.
   bottom_separator_->setVisible(draw_bottom_separator_ && visual == DragVisual::kNormal);

    if (visual == DragVisual::kDimmed) {
        // Same dim treatment as ModulationManager's mapping-mode overlay.
        tint_overlay_->setColor(juce::Colours::black);
        tint_overlay_->setAlpha(0.45f);
    }
    else if (visual == DragVisual::kDropTarget) {
        // The overlapped module gets the same white sheen rather than snapping to
        // full brightness, so it reads as "engaged" but stays subordinate.
        tint_overlay_->setColor(juce::Colours::white);
        tint_overlay_->setAlpha(0.08f);
    }
    else
        tint_overlay_->setAlpha(0.0f);

    const juce::Colour accent = findColour(drag_accent_color_id_, true);
    highlight_border_->setColor(accent);
    if (visual == DragVisual::kDragged) {
        highlight_border_->setThickness(3.0f, true);
        highlight_border_->setAlpha(1.0f);
    }
    else if (visual == DragVisual::kDropTarget) {
        highlight_border_->setThickness(1.0f, true);
        highlight_border_->setAlpha(0.35f);
    }
    else
        highlight_border_->setAlpha(0.0f);
}

void ModuleSection::buttonClicked(juce::Button *button) {
    if (button == exit_button_.get()) {
        this->setVisible(false);
        undo.beginNewTransaction();
        state.getParent().removeChild(state,&undo);
    }
}

void ModuleSection::paintBackground(juce::Graphics &g) {
    //paintContainer(g);
    paintBorder(g);
    //paintKnobShadows(g);
    // paintChildrenBackgrounds(g);
}
