//
// Created by Davis Polito on 11/19/24.
//


#include "ModulationSection.h"
#include "modulation_button.h"
#include "modulation_manager.h"
ModulationSection::ModulationSection( const juce::ValueTree &v, std::unique_ptr<SynthSection> editor, juce::UndoManager& um)
                        : SynthSection(editor->getName()),
                        state(v),
                        _view(std::move(editor)),
                        mod_button(new ModulationButton("mod")),
                        undo(um) // this is the dragged connector
{
    setComponentID(_view->getName());
    addModulationButton(mod_button, false);
    mod_button->setAlwaysOnTop(true);
    addSubSection(_view.get());
    if (auto* parameters = dynamic_cast<electrosynth::ParametersView*>(_view.get()))
        parameters->setVerticallyCenterKnobs(true);
    exit_button_ = std::make_shared<OpenGlShapeButton>("Exit");
    addAndMakeVisible(exit_button_.get());
    addOpenGlComponent(exit_button_->getGlComponent());
    exit_button_->addListener(this);
    exit_button_->setShape(Paths::exitX());
}

ModulationSection::~ModulationSection() = default;

juce::String ModulationSection::getModulatorType() const {
    return state.getProperty(IDs::type).toString();
}

void ModulationSection::setAreaSkinOverride(Skin::SectionOverride skin_override) {
    setSkinOverride(skin_override);
    if (_view != nullptr)
        _view->setSkinOverride(skin_override);
}

void ModulationSection::paintBackground(juce::Graphics &g){
    SynthSection::paintBackground(g);
}

void ModulationSection::resized() {

    _view->setBounds(getLocalBounds());
    if (mod_button->getParentComponent() == this)
        mod_button->setBounds(_view->getRight() - 40, getY(),40,40);
    exit_button_->setBounds(0,0, 30,30);

    SynthSection::resized();
}


void ModulationSection::addModButtonListener(ModulationManager* manager) const {
    mod_button->addListener(manager);
}

void ModulationSection::buttonClicked(juce::Button *button) {
    if (button == exit_button_.get()) {
        undo.beginNewTransaction();
        state.getParent().removeChild(state,&undo);
    }
}
