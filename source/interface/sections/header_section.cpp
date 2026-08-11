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

  SynthSection::resized();
}

void HeaderSection::reset() {
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

