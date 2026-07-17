//
// Created by Davis Polito on 6/5/25.
//

#include "MasterEnvelopeSection.h"
#include "main_section.h"
#include "modulation_button.h"


MasterVoiceEnvelopeSection:: MasterVoiceEnvelopeSection(const juce::ValueTree& v, juce::UndoManager &um, OpenGlWrapper &open_gl,
                                                        SynthGuiData * data, std::unique_ptr<SynthSection>&& view) : SynthSection("MasterEnv"), mod_button(std::make_unique<ModulationButton>("mod_masterenv")), master_voice_envelope(std::move(view)) {
    setName("Master Voice Envelope");
    setSkinOverride(Skin::kMasterEnv);
    setSidewaysHeading(false);
    header_body_ = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, "master_voice_envelope_header");
    header_body_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(header_body_, true);

    header_title_ = std::make_shared<PlainTextComponent>("master_voice_envelope_title", getName());
    header_title_->setFontType(PlainTextComponent::kLight);
    header_title_->setJustification(juce::Justification::centred);
    header_title_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(header_title_);

    master_voice_envelope->setName("VCA");
    setComponentID(master_voice_envelope->getName());
    master_voice_envelope->setSkinOverride(Skin::kMasterEnv);
    addSubSection(master_voice_envelope.get());
    if (auto* parameters = dynamic_cast<electrosynth::ParametersView*>(master_voice_envelope.get()))
        parameters->setVerticallyCenterKnobs(true);
    addModulationButton(mod_button, false);
    mod_button->setAlwaysOnTop(true);
}

void MasterVoiceEnvelopeSection::resized() {
    master_voice_envelope->setBounds(getLocalBounds());
    mod_button->setBounds(0, 0, 40, 40);
    SynthSection::resized();

    const int title_width = static_cast<int>(getTitleWidth());
    header_body_->setBounds(0, 0, getWidth(), title_width);
    header_body_->setColor(findColour(Skin::kBodyHeading, true));
    header_body_->setVisible(false);
    header_title_->setBounds(0, 0, getWidth(), title_width);
    header_title_->setText(getName());
    header_title_->setTextSize(size_ratio_ * 14.0f);
    header_title_->setColor(findColour(Skin::kHeadingText, true));
    header_title_->setVisible(false);
}

void MasterVoiceEnvelopeSection::paintBackground(Graphics &g) {
    paintContainer(g);
    paintKnobShadows(g);
    paintChildrenBackgrounds(g);
    g.setColour(findColour(Skin::kBorder, true));
    paintBorder(g);
}
