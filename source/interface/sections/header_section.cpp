/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "header_section.h"

#include "fonts.h"
#include <memory>


LogoSection::LogoSection() : SynthSection("logo_section") {
#if !defined(NO_TEXT_ENTRY)
  logo_button_ = std::make_unique<LogoButton>("logo");

  addAndMakeVisible(logo_button_.get());
  addOpenGlComponent(logo_button_->getImageComponent());
  logo_button_->addListener(this);
#endif

  setSkinOverride(Skin::kLogo);
}

void LogoSection::resized() {
  int logo_padding_y = kLogoPaddingY * size_ratio_;
  int logo_height = getHeight() - 2 * logo_padding_y;
  int logo_padding_x = (getWidth() - logo_height) / 2;
  if (logo_button_)
    logo_button_->setBounds(logo_padding_x, logo_padding_y, logo_height, logo_height);
}

void LogoSection::paintBackground(Graphics& g) {
  if (logo_button_) {
    logo_button_->setRingColors(findColour(Skin::kWidgetPrimary1, true), findColour(Skin::kWidgetPrimary2, true));
    logo_button_->setLetterColors(findColour(Skin::kWidgetSecondary1, true), findColour(Skin::kWidgetSecondary2, true));
  }
}

void LogoSection::buttonClicked(Button* clicked_button) {
  for (Listener* listener : listeners_)
    listener->showAboutSection();
}

HeaderSection::HeaderSection() : SynthSection("header_section"), tab_offset_(0), body_(new OpenGlQuad(Shaders::kRoundedRectangleFragment)) {


    addOpenGlComponent(body_);
    logo_section_ = std::make_unique<LogoSection>();
    logo_section_->setAlwaysOnTop(true);
    logo_section_->addListener(this);
    addSubSection(logo_section_.get());

  setAlwaysOnTop(true);
  setSkinOverride(Skin::kHeader);
}

void HeaderSection::paintBackground(Graphics& g) {
  paintContainer(g);

  g.setColour(findColour(Skin::kBody, true));
  int logo_section_width = 32.0 + getPadding();
  g.fillRect(0, 0, logo_section_width, getHeight());

  paintKnobShadows(g);
  paintChildrenBackgrounds(g);

  g.saveState();

  g.restoreState();


}

void HeaderSection::resized() {
  static constexpr float kTextHeightRatio = 0.3f;
  static constexpr float kPaddingLeft = 0.25f;


  int height = getHeight();
  int width = getWidth();
body_->setBounds(getLocalBounds());
    body_->setRounding(findValue(Skin::kBodyRounding));
    body_->setColor(findColour(Skin::kBody, true));


    int logo_width = findValue(Skin::kModulationButtonWidth);
    logo_section_->setBounds(0, -10, logo_width , height );

  int preset_selector_width = width / 3;
  int preset_selector_height = height * 0.6f;
  int preset_selector_buffer = (height - preset_selector_height) * 0.5f;
  int preset_selector_x = (getWidth() - preset_selector_width + 2 * preset_selector_height) / 2;

  int component_padding = 12 * size_ratio_;
  SynthSection::resized();
}

void HeaderSection::reset() {
//  if (preset_selector_)
//    //synth_preset_selector_->resetText();
}



void HeaderSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == exit_temporary_button_.get()) {
//    for (Listener* listener : listeners_)
//      listener->clearTemporaryTab(tab_selector_->getValue());
//    setTemporaryTab("");
  }

  else
    SynthSection::buttonClicked(clicked_button);
}

void HeaderSection::sliderValueChanged(Slider* slider) {

    SynthSection::sliderValueChanged(slider);
}

