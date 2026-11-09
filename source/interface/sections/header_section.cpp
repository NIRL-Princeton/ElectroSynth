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

    sendToDevicePresetNumberEditor = std::make_unique<OpenGlTextEditor>("header_preset_number_send");
    addOpenGlComponent (sendToDevicePresetNumberEditor->getImageComponent());
    addAndMakeVisible (sendToDevicePresetNumberEditor.get());
    sendToDevicePresetNumberEditor->addListener (this);

    const auto editors = {sendToDeviceTextEditor.get(), sendToDevicePresetNumberEditor.get()};

    const auto buttonTextColourOff = findColour(Skin::kIconButtonOff);
    const auto buttonTextColourOn = findColour(Skin::kIconButtonOffHover);
    const auto buttonTextBackgroundColour = findColour(Skin::kTextComponentBackground);

    for (const auto editor : editors)
    {
        editor->setColour (Skin::kTextEditorBackground, buttonTextBackgroundColour);
        editor->setColour (juce::TextEditor::textColourId, buttonTextColourOn);
        editor->setColour (juce::TextEditor::highlightColourId, buttonTextColourOff);
        editor->setLookAndFeel(TextLookAndFeel::instance());
        editor->redoImage();
    }


    sendToDeviceTextEditor->setTextToShowWhenEmpty ("Device name...", juce::Colours::darkgrey);
    sendToDevicePresetNumberEditor->setTextToShowWhenEmpty ("0...255", juce::Colours::darkgrey);

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
    sendToDevicePresetNumberEditor->setBounds(bounds.removeFromRight (60 * ratio).withHeight (30 * ratio).withY(getLocalBounds().getHeight()/4));

    bounds.removeFromRight (Skin::kLargePadding);
    sendToDeviceTextEditor->setBounds(bounds.removeFromRight (150 * ratio).withHeight (30 * ratio).withY(getLocalBounds().getHeight()/4));

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

static juce::String ensureASCIINumbersOnly(juce::String name)
{
    juce::String asciiOnlyString;
    for (auto c : name)
    {
        if (c >= '0' && c <= '9')
        {
            asciiOnlyString += static_cast<juce::juce_wchar>(c);
        }
    }
    return asciiOnlyString;
}

static juce::String processDeviceText(juce::String name) {
    name = ensureASCIIOnly(name);
    if (name.length() > 14) name = name.substring (0, 14);
    return name;
}

static juce::String processPresetNumberText(juce::String number)
{
    number = ensureASCIINumbersOnly (ensureASCIIOnly (number));
    if (number.length() > 3) number = number.substring (0, 3);
    number = (number.getIntValue() < 256) ? number : juce::String(255);
    return number;
}

void HeaderSection::textEditorTextChanged(juce::TextEditor& editor) {

    if (&editor == sendToDeviceTextEditor.get()) {
        const auto name = processDeviceText(editor.getText());

        for (Listener* listener : listeners_)
            listener->sendToDeviceStringChanged(name);
    }
    else if (&editor == sendToDevicePresetNumberEditor.get())
    {
        const auto number = processPresetNumberText(editor.getText()).getIntValue();
        for (Listener* listener : listeners_)
            listener->sendToDevicePresetNumberChanged(number);
    }
}

void HeaderSection::textEditorReturnKeyPressed(juce::TextEditor& editor) {

    if (&editor == sendToDeviceTextEditor.get()) {
        const auto name = processDeviceText(editor.getText());
        editor.setText(name);
    }
    else if (&editor == sendToDevicePresetNumberEditor.get())
    {
        const auto number = processPresetNumberText (editor.getText());
        editor.setText(number);
    }
}

void HeaderSection::textEditorFocusLost(juce::TextEditor& editor) {
    if (&editor == sendToDeviceTextEditor.get()) {
        const auto name = processDeviceText(editor.getText());
        editor.setText(name);
    }
    else if (&editor == sendToDevicePresetNumberEditor.get()) {
        const auto number = processPresetNumberText(editor.getText());
        editor.setText(number);
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

