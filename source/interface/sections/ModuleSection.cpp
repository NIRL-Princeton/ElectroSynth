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

ModuleSection::ModuleSection(const juce::ValueTree &v, std::unique_ptr<SynthSection> editor, juce::UndoManager& um)
    : SynthSection(editor->getName()), state(v), _view(std::move(editor)), undo(um) {

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
}

ModuleSection::~ModuleSection() = default;

void ModuleSection::setAreaSkinOverride(Skin::SectionOverride skin_override) {
    setSkinOverride(skin_override);
    if (_view != nullptr)
        _view->setSkinOverride(skin_override);
}

int ModuleSection::getPreferredHeight() const {
    return (_view != nullptr ? _view->getPreferredHeight() : 0)
           + kHeaderHeight
           + kContentBottomPadding;
}

int ModuleSection::refreshHeight() {
    height = getPreferredHeight();
    return height;
}

void ModuleSection::resized() {
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

    bottom_separator_->setBounds(0, std::max(0, getHeight() - 1), getWidth(), 2);
    bottom_separator_->setColor(findColour(Skin::kBodyHeading, true));
    bottom_separator_->setVisible(draw_bottom_separator_);

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
    // g.setColour(findColour(Skin::kBorder, true));
    //paintBorder(g);
    //paintKnobShadows(g);
    // paintChildrenBackgrounds(g);
}
