#include "header_section.h"
#include "fonts.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include "text_look_and_feel.h"
#include <memory>

HeaderSection::HeaderSection() : SynthSection("header_section"), tab_offset_(0), body_(new OpenGlQuad(Shaders::kRoundedRectangleFragment)) {

    addOpenGlComponent(body_);

    audioSettingsButton = std::make_unique<OpenGlTextButton>("header_audio");
    addOpenGlComponent(audioSettingsButton->getGlComponent());
    addAndMakeVisible(audioSettingsButton.get());
    audioSettingsButton->addListener(this);
    audioSettingsButton->setLookAndFeel(TextLookAndFeel::instance());
    audioSettingsButton->setButtonText("Audio/MIDI Settings");

    sendToDeviceButton = std::make_unique<OpenGlTextButton>("header_send");
    addOpenGlComponent(sendToDeviceButton->getGlComponent());
    addAndMakeVisible(sendToDeviceButton.get());
    sendToDeviceButton->addListener(this);
    sendToDeviceButton->setLookAndFeel(TextLookAndFeel::instance());
    sendToDeviceButton->setButtonText("Send to Device");


    sendToDeviceTextEditor  = std::make_unique<OpenGlTextEditor>("header_text_send");
    addOpenGlComponent (sendToDeviceTextEditor->getImageComponent());
    addAndMakeVisible(sendToDeviceTextEditor.get());
    sendToDeviceTextEditor->addListener (this);

    const auto buttonTextColourOff = findColour(Skin::kIconButtonOff);
    const auto buttonTextColourOn = findColour(Skin::kIconButtonOffHover);
    const auto buttonTextBackgroundColour = findColour(Skin::kTextComponentBackground);
    sendToDeviceTextEditor->setColour (Skin::kTextEditorBackground, buttonTextBackgroundColour);
    sendToDeviceTextEditor->setColour (juce::TextEditor::textColourId, buttonTextColourOn);
    sendToDeviceTextEditor->setColour (juce::TextEditor::highlightColourId, buttonTextColourOff);
    sendToDeviceTextEditor->setLookAndFeel(TextLookAndFeel::instance());
    sendToDeviceTextEditor->setTextToShowWhenEmpty ("Device to send to...", juce::Colours::darkgrey);
    sendToDeviceTextEditor->redoImage();

    setAlwaysOnTop(true);
    setSkinOverride(Skin::kHeader);
}

void HeaderSection::paintBackground(Graphics& g) {
  paintContainer(g);
  g.setColour(findColour(Skin::kBody, true));
  // paintChildrenBackgrounds(g);
  g.saveState();
  g.restoreState();
}

void HeaderSection::resized() {
    float ratio = getSizeRatio();
    auto bounds = getLocalBounds();
    body_->setBounds(bounds);
    body_->setRounding(findValue(Skin::kBodyRounding));
    body_->setColor(findColour(Skin::kBody, true));

    bounds.removeFromLeft (Skin::kLargePadding);
    audioSettingsButton->setBounds(bounds.removeFromLeft (150 * ratio).withHeight (30 * ratio).withY(getLocalBounds().getHeight()/4));

    bounds.removeFromRight (Skin::kLargePadding);
    sendToDeviceButton->setBounds(bounds.removeFromRight (120 * ratio).withHeight (30 * ratio).withY(getLocalBounds().getHeight()/4));

    bounds.removeFromRight (Skin::kLargePadding);
    sendToDeviceTextEditor->setBounds(bounds.removeFromRight (240 * ratio).withHeight (30 * ratio).withY(getLocalBounds().getHeight()/4));

  SynthSection::resized();
}

void HeaderSection::reset() {
}

static juce::String ensureASCIIOnly(juce::String name)
{
    juce::String asciiOnlyString = "";

    for (auto p = name.getCharPointer(); !p.isEmpty(); ++p)
    {
        juce::juce_wchar c = *p; // Get the character

        // Example: Print or inspect 'c'
        if (c >= 0 && c <= 127)
        {
            asciiOnlyString += static_cast<juce::juce_wchar>(c);
        }
    }
    return asciiOnlyString;
}

static juce::String processDeviceText(juce::String name) {
    name = ensureASCIIOnly(name);
    if (name.length() > 14) name = name.substring (0, 14);
    return name.paddedRight (' ', 14);
}

void HeaderSection::textEditorTextChanged(juce::TextEditor& editor) {

    if (&editor == sendToDeviceTextEditor.get()) {
        const auto name = processDeviceText(editor.getText());

        for (Listener* listener : listeners_)
            listener->sendToDeviceStringChanged(name);
    }
}

void HeaderSection::textEditorReturnKeyPressed(juce::TextEditor& editor) {

    if (&editor == sendToDeviceTextEditor.get()) {
        auto name = processDeviceText(sendToDeviceTextEditor->getText());
        sendToDeviceTextEditor->setText(name);
    }
}

void HeaderSection::textEditorFocusLost(juce::TextEditor& editor) {
    if (&editor == sendToDeviceTextEditor.get()) {
        auto name = processDeviceText(sendToDeviceTextEditor->getText());
        sendToDeviceTextEditor->setText(name);
    }
}

void HeaderSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == exit_temporary_button_.get()) {
  }
  else if (clicked_button == audioSettingsButton.get()) {
      for (Listener* listener : listeners_)
          listener->showAboutSection();
  }
  else if (clicked_button == sendToDeviceButton.get()) {
      for (Listener* listener : listeners_)
          listener->sendToDeviceRequested();
  }
  else
    SynthSection::buttonClicked(clicked_button);
}

void HeaderSection::sliderValueChanged(Slider* slider) {

    SynthSection::sliderValueChanged(slider);
}

