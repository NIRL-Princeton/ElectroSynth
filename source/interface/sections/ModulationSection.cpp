//
// Created by Davis Polito on 11/19/24.
//


#include "ModulationSection.h"
#include "connection_button.h"
#include "mapping_manager.h"
#include "connection_slots.h"

ModulationSection::ModulationSection( const juce::ValueTree &v, std::unique_ptr<SynthSection> editor, juce::UndoManager& um)
                        : SynthSection(editor->getName()), state(v), _view(std::move(editor)), undo(um) {
    setComponentID(_view->getName());

    electrosynth::EndpointDescriptor sourceEndpoint {
        .address {
            .type = electrosynth::ConnectionType::Modulation,
            .nodeId = state.getProperty(IDs::nodeID).toString(),
            .endpointId = getComponentID() + "_mod",
            .direction = electrosynth::EndpointDirection::Source
        },
        .capabilities {
            .maxIncomingConnections = 0
        }
    };

    mod_button = std::make_shared<ConnectionButton>("mod", std::move(sourceEndpoint));

    addModulationButton(mod_button, true);
    connection_slots_ = std::make_unique<ConnectionSlots>(*mod_button);
    addSubSection(connection_slots_.get());
    connection_slots_->setConnections({});

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
    constexpr int arrowSize = 24;
    constexpr int rightPadding = 5;
    constexpr int bottomPadding = 5;
    constexpr int slotGap = 2;

    // Keep the editor border on the module's outer edge. Trimming its height
    // placed that border directly above the routing row as a horizontal line.
    _view->setBounds(getLocalBounds());

    mod_button->setBounds(
        getWidth() - rightPadding - arrowSize,
        getHeight() - bottomPadding - arrowSize,
        arrowSize,
        arrowSize);

    connection_slots_->setBounds(
        mod_button->getX() - slotGap - ConnectionSlots::kPreferredWidth,
        mod_button->getY(),
        ConnectionSlots::kPreferredWidth,
        arrowSize);


    exit_button_->setBounds(0,0, 30,30);
    SynthSection::resized();
}


void ModulationSection::addModButtonListener(MappingManager* manager) const {
    mod_button->addListener(manager);
}

void ModulationSection::buttonClicked(juce::Button *button) {
    if (button == exit_button_.get()) {
        undo.beginNewTransaction();
        state.getParent().removeChild(state,&undo);
    }
}
