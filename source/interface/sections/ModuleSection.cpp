//
// Created by Davis Polito on 10/22/24.
//

#include "ModuleSection.h"
ModuleSection::ModuleSection(const juce::ValueTree &v, std::unique_ptr<SynthSection> editor, juce::UndoManager& um) : SynthSection(editor->getName()), state(v), _view(std::move(editor)), undo(um)
{

    // background_ left null — module content is drawn by EffectModuleSection's 2x HiDPI background image
    // background_->paintEntireComponent(false);
    // background_->setInterceptsMouseClicks(false, false);
    setComponentID(_view->getName());
    addSubSection(_view.get());
    _view->setAlwaysOnTop(true);
    // setInterceptsMouseClicks(true,false);

    exit_button_ = std::make_unique<OpenGlShapeButton>("Exit");
   addAndMakeVisible(exit_button_.get());
    addOpenGlComponent(exit_button_->getGlComponent());
    exit_button_->addListener(this);
    exit_button_->setShape(Paths::exitX());
    exit_button_->setAlwaysOnTop(true);


}

ModuleSection::~ModuleSection() = default;

void ModuleSection::paintBackground(juce::Graphics &g)
{
    paintChildrenBackgrounds(g);
}

void ModuleSection::resized()
{
    auto local = getLocalBounds();
    auto area = local.removeFromTop(25);
  _view->setBounds(local);
   int knob_y2 =0;
   SynthSection::resized();
    // background_->setBounds(getLocalBounds());
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
        undo.beginNewTransaction();
        state.getParent().removeChild(state,&undo);
    }
}