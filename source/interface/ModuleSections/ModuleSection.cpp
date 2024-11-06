//
// Created by Davis Polito on 10/22/24.
//

#include "ModuleSection.h"
ModuleSection::ModuleSection(juce::String name, const juce::ValueTree &v, electrosynth::ParametersViewEditor* editor) : SynthSection(name), state(v)
{
    _view_editor = editor;
    _view = &_view_editor->view;

    addSubSection(_view);
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
}

//void ModuleSection::setParametersViewEditor (electrosynth::ParametersViewEditor&& editor)
//{
//   _view_editor = editor;
//   addSubSection(_view);
//
//}