//
// Created by Davis Polito on 10/22/24.
//

#include "ModuleSection.h"
ModuleSection::ModuleSection(const juce::ValueTree &v, electrosynth::ParametersView* editor) : SynthSection(editor->getName()), state(v), _view(editor)
{
    setComponentID(editor->getName());
    addSubSection(_view.get());
    setInterceptsMouseClicks(false, true);

    exit_button_ = std::make_shared<OpenGlShapeButton>("Exit");
    addAndMakeVisible(exit_button_.get());
    addOpenGlComponent(exit_button_->getGlComponent());
    exit_button_->addListener(this);
    exit_button_->setShape(Paths::exitX());
}

ModuleSection::~ModuleSection() = default;

void ModuleSection::paintBackground(juce::Graphics &g)
{
   SynthSection::paintBackground(g);
}

void ModuleSection::resized()
{
   int widget_margin = findValue(Skin::kWidgetMargin);
   int title_width = getTitleWidth();
   int section_height = getKnobSectionHeight();

   Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
   Rectangle<int> knobs_area = getDividedAreaBuffered(bounds, 2, 1, widget_margin);
   Rectangle<int> settings_area = getDividedAreaUnbuffered(bounds, 4, 0, widget_margin);
   _view->setBounds(getLocalBounds());
   int knob_y2 =0;
   SynthSection::resized();
    exit_button_->setBounds(0,0, 50,50);
}

//void ModuleSection::setParametersViewEditor (electrosynth::ParametersViewEditor&& editor)
//{
//   _view_editor = editor;
//   addSubSection(_view);
//
//}
void ModuleSection::buttonClicked(juce::Button *button) {
    if (button == exit_button_.get()) {
        state.getParent().removeChild(state,nullptr);
    }
}