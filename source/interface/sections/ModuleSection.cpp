//
// Created by Davis Polito on 10/22/24.
//

// ModuleSection.cpp is the wrapper for one entire module (one oscillator, string, filter, etc.). It owns the module's actual
// editor view (ParametersView) and gives it a header, border/background, exit button, draft behavior, and a height.

#include "ModuleSection.h"
#include "SoundModuleSection.h"

namespace {
    constexpr int kExitButtonSize = 25;
    constexpr int kExitButtonRightOffset = 50;
}

ModuleSection::ModuleSection(const juce::ValueTree &v, std::unique_ptr<SynthSection> editor, juce::UndoManager& um)
    : SynthSection(editor->getName()), state(v), _view(std::move(editor)), undo(um) {

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
    if (auto* sound_module = findParentComponentOfClass<SoundModuleSection>()) {
        const auto module_bounds_in_sound_module = sound_module->getLocalArea(this, getLocalBounds());
        const int sound_module_exit_x = sound_module->getWidth() - kExitButtonRightOffset;
        exit_x = juce::jlimit(0, std::max(0, getWidth() - kExitButtonSize),
                              sound_module_exit_x - module_bounds_in_sound_module.getX());
    }
    exit_button_->setBounds(exit_x, (kHeaderHeight - kExitButtonSize) / 2, kExitButtonSize, kExitButtonSize);

    bottom_separator_->setBounds(0, std::max(0, getHeight() - 1), getWidth(), 2);
    bottom_separator_->setColor(findColour(Skin::kBodyHeading, true));
    bottom_separator_->setVisible(draw_bottom_separator_);

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
