//
// Created by Davis Polito on 11/19/24.
//


#include "ModulationSection.h"
#include "modulation_button.h"
#include "modulation_manager.h"
ModulationSection::ModulationSection( const juce::ValueTree &v, electrosynth::ParametersView* editor) : SynthSection(editor->getName()), state(v), _view(editor),
mod_button(new ModulationButton("mod"))
{
    setComponentID(editor->getName());
    addModulationButton(mod_button );
    addAndMakeVisible(mod_button.get());
    mod_button->setAlwaysOnTop(true);
    addSubSection(_view.get());
}

ModulationSection::~ModulationSection() = default;

void ModulationSection::paintBackground(juce::Graphics &g)
{
        SynthSection::paintBackground(g);
    // g.fillAll(juce::Colours::green);
}

void ModulationSection::resized()
{
    int widget_margin = findValue(Skin::kWidgetMargin);
    int title_width = getTitleWidth();
    int section_height = getKnobSectionHeight();

    Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
    Rectangle<int> knobs_area = getDividedAreaBuffered(bounds, 2, 1, widget_margin);
    Rectangle<int> settings_area = getDividedAreaUnbuffered(bounds, 4, 0, widget_margin);
    _view->setBounds(getLocalBounds());
    mod_button->setBounds(_view->getRight() - 40, getY(),40,40);
    int knob_y2 =0;
    SynthSection::resized();
}


void ModulationSection::addModButtonListener(ModulationManager* manager)
{
    mod_button->addListener(manager);
}

//void ModulationSection::setParametersViewEditor (electrosynth::ParametersViewEditor&& editor)
//{
//   _view_editor = editor;
//   addSubSection(_view);
//
//}