//
// Created by Davis Polito on 10/22/24.
//

#include "ModuleSection.h"
ModuleSection::ModuleSection(const juce::ValueTree &v, std::unique_ptr<SynthSection> editor) : SynthSection(editor->getName()), state(v), _view(std::move(editor))
{
    setComponentID(_view->getName());
    addSubSection(_view.get());
    setInterceptsMouseClicks(false, true);

    exit_button_ = std::make_unique<OpenGlShapeButton>("Exit");
    addAndMakeVisible(exit_button_.get());
    addOpenGlComponent(exit_button_->getGlComponent());
    exit_button_->addListener(this);
    exit_button_->setShape(Paths::exitX());
    background_ = std::make_unique<OpenGlImageComponent>("background");
    addOpenGlComponent(background_);
    background_->setComponent(this);
    background_->paintEntireComponent(false);

}

ModuleSection::~ModuleSection() = default;

void ModuleSection::paintBackground(juce::Graphics &g)
{

    paintContainer(g);
    paintHeadingText(g);

    paintKnobShadows(g);
    paintChildrenBackgrounds(g);    paintBorder(g);
   //SynthSection::paintBackground(g);
}

void ModuleSection::resized()
{
   _view->setBounds(getLocalBounds());
   int knob_y2 =0;
   SynthSection::resized();
    exit_button_->setBounds(getLocalBounds().getRight() - 50,0, 25,25);
}

//void ModuleSection::setParametersViewEditor (electrosynth::ParametersViewEditor&& editor)
//{
//   _view_editor = editor;
//   addSubSection(_view);
//
//}
void ModuleSection::buttonClicked(juce::Button *button) {
    if (button == exit_button_.get()) {
        this->setVisible(false);
        //DBG("state " state.getParent())
        state.getParent().removeChild(state,nullptr);
    }
}