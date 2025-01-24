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

#include "synth_slider.h"

#include <cmath>

#include "skin.h"
#include "fonts.h"
#include "curve_look_and_feel.h"
#include "FullInterface.h"

#include "chowdsp_parameters/ParamUtils/chowdsp_ParameterTypes.h"
#include "chowdsp_plugin_state/Backend/chowdsp_PluginState.h"
#include "synth_gui_interface.h"
#include "synth_section.h"
#include "synth_section.h"
void OpenGlSliderQuad::init(OpenGlWrapper& open_gl) {
  if (slider_->isModulationKnob())
    setFragmentShader(Shaders::kModulationKnobFragment);
  else if (slider_->isRotaryQuad())
    setFragmentShader(Shaders::kRotarySliderFragment);
  else if (slider_->isHorizontalQuad())
    setFragmentShader(Shaders::kHorizontalSliderFragment);
  else
    setFragmentShader(Shaders::kVerticalSliderFragment);

  OpenGlQuad::init(open_gl);
}

void OpenGlSliderQuad::paintBackground(juce::Graphics& g) {
  slider_->redoImage(false);
}

int OpenGlSlider::getLinearSliderWidth() {
  int total_width = isHorizontal() ? getHeight() : getWidth();
  int extra = total_width % 2;
  return std::floor(SynthSlider::kLinearWidthPercent * total_width * 0.5f) * 2.0f + extra;
}

void OpenGlSlider::setSliderDisplayValues() {
  if (isModulationKnob()) {
    float radius = 1.0f - 1.0f / getWidth();
    slider_quad_->setQuad(0, -radius, -radius, 2.0f * radius, 2.0f * radius);
  }
  else if (isRotaryQuad()) {
    float thickness = findValue(Skin::kKnobArcThickness);
    float size = findValue(Skin::kKnobArcSize) * getKnobSizeScale() + thickness;
    float offset = findValue(Skin::kKnobOffset);
    float radius_x = (size + 0.5f) / getWidth();
    float center_y = 2.0f *offset / getHeight();
    float radius_y = (size + 0.5f) / getHeight();
    slider_quad_->setQuad(0, -radius_x, -center_y - radius_y, 2.0f * radius_x, 2.0f * radius_y);
    slider_quad_->setThumbAmount(findValue(Skin::kKnobHandleLength));
  }
  else if (isHorizontalQuad()) {
    float margin = 2.0f * (findValue(Skin::kWidgetMargin) - 0.5f) / getWidth();
    slider_quad_->setQuad(0, -1.0f + margin, -1.0f, 2.0f - 2.0f * margin, 2.0f);
  }
  else if (isVerticalQuad()) {
    float margin = 2.0f * (findValue(Skin::kWidgetMargin) - 0.5f) / getHeight();
    slider_quad_->setQuad(0, -1.0f, -1.0f + margin, 2.0f, 2.0f - 2.0f * margin);
  }
}

void OpenGlSlider:: redoImage(bool skip_image) {
  static constexpr float kRoundingMult = 0.4f;
  static constexpr float kRotaryHoverBoost = 1.4f;
  if (getWidth() <= 0 || getHeight() <= 0)
    return;

  bool horizontal = isHorizontalQuad();
  bool vertical = isVerticalQuad();
  if (modulation_amount_) {
    slider_quad_->setModColor(mod_color_);
    slider_quad_->setBackgroundColor(background_color_);
  }
  else {
    slider_quad_->setModColor(juce::Colours::transparentBlack);
    slider_quad_->setBackgroundColor(juce::Colours::transparentBlack);
  }

  if (isModulationKnob()) {
    slider_quad_->setActive(true);
    float t = getValue();
    slider_quad_->setThumbColor(thumb_color_);

    if (t > 0.0f) {
      slider_quad_->setShaderValue(0, electrosynth::utils::interpolate(electrosynth::kPi, -electrosynth::kPi, t));
      slider_quad_->setColor(unselected_color_);
      slider_quad_->setAltColor(selected_color_);
    }
    else {
      slider_quad_->setShaderValue(0, electrosynth::utils::interpolate(-electrosynth::kPi, electrosynth::kPi, -t));
      slider_quad_->setColor(selected_color_);
      slider_quad_->setAltColor(unselected_color_);
    }

    if (isMouseOverOrDragging())
      slider_quad_->setThickness(1.8f);
    else
      slider_quad_->setThickness(1.0f);
  }
  else if (isRotaryQuad()) {
    slider_quad_->setActive(true);
    float arc = slider_quad_->getMaxArc();
    float t = valueToProportionOfLength(getValue());
    slider_quad_->setShaderValue(0, electrosynth::utils::interpolate(-arc, arc, t));
    slider_quad_->setColor(selected_color_);
    slider_quad_->setAltColor(unselected_color_);
    slider_quad_->setThumbColor(thumb_color_);
    slider_quad_->setStartPos(bipolar_ ? 0.0f : -electrosynth::kPi);

    float thickness = findValue(Skin::kKnobArcThickness);
    if (isMouseOverOrDragging())
      thickness *= kRotaryHoverBoost;
    slider_quad_->setThickness(thickness);
  }
  else if (horizontal || vertical) {
    slider_quad_->setActive(true);
    float t = valueToProportionOfLength(getValue());
    slider_quad_->setShaderValue(0, t);
    slider_quad_->setColor(selected_color_);
    slider_quad_->setAltColor(unselected_color_);
    slider_quad_->setThumbColor(thumb_color_);
    slider_quad_->setStartPos(bipolar_ ? 0.0f : -1.0f);

    int total_width = horizontal ? getHeight() : getWidth();
    float slider_width = getLinearSliderWidth();
    float handle_width = SynthSlider::kLinearHandlePercent * total_width;
    if (isMouseOverOrDragging()) {
      int boost = std::round(slider_width / 8.0f) + 1.0f;
      slider_quad_->setThickness(slider_width + 2 * boost);
    }
    else
      slider_quad_->setThickness(slider_width);
    slider_quad_->setRounding(slider_width * kRoundingMult);
    slider_quad_->setThumbAmount(handle_width);
  }
  if (!skip_image) {
//      juce::Image _image(juce::Image::SingleChannel, getWidth(), getHeight(), true);
//      juce::Graphics g(_image);
//      g.setColour(findColour(Skin::kRotaryBody,true));
//      g.fillAll();
//      image_component_.image().setOwnImage(_image);
//    image_component_.draw_image_ = std::make_unique<juce::Image>(_image);
    image_component_->setActive(true);
    image_component_->redrawImage(true, false);
  }
}

void OpenGlSlider::setColors() {
  if (getWidth() <= 0)
    return;

  thumb_color_ = getThumbColor();
  selected_color_ = getSelectedColor();
  unselected_color_ = getUnselectedColor();
  background_color_ = getBackgroundColor();
  mod_color_ = getModColor();
}

SynthSlider::SynthSlider(juce::String name) : OpenGlSlider(name), show_popup_on_hover_(false), scroll_enabled_(true),
                                                                              bipolar_modulation_(false), stereo_modulation_(false),
                                                                              bypass_modulation_(false), modulation_bar_right_(true),
                                                                              snap_to_value_(false), hovering_(false),
                                                                              has_parameter_assignment_(false),
                                                                              use_suffix_(true), snap_value_(0.0),
                                                                              text_height_percentage_(0.0f),
                                                                              sensitivity_(kDefaultSensitivity),
                                                                              popup_placement_(juce::BubbleComponent::below),
                                                                              modulation_control_placement_(juce::BubbleComponent::below),
                                                                              max_display_characters_(kDefaultFormatLength),
                                                                              max_decimal_places_(kDefaultFormatDecimalPlaces), shift_index_amount_(0),
                                                                              shift_is_multiplicative_(false), mouse_wheel_index_movement_(1.0),
                                                                              text_entry_width_percent_(kDefaultTextEntryWidthPercent),
                                                                              text_entry_height_percent_(kDefaultTextEntryHeightPercent),
                                                                              display_multiply_(0.0f), display_exponential_base_(2.0f),
                                                                              string_lookup_(nullptr), extra_modulation_target_(nullptr),
                                                                              synth_interface_(nullptr) /*attachment(param,pluginState, *this)*/{
    //setAttachment(param, pluginState);
    setComponentID (name);
    text_entry_ = std::make_unique<OpenGlTextEditor>(name);
    text_entry_->setMonospace();
    text_entry_->setMultiLine(false);
    text_entry_->setScrollToShowCursor(false);
    text_entry_->addListener(this);
    text_entry_->setSelectAllWhenFocused(true);
    text_entry_->setKeyboardType(juce::TextEditor::numericKeyboard);
    text_entry_->setJustification(juce::Justification::centred);
    text_entry_->setAlwaysOnTop(true);
    text_entry_->getImageComponent()->setAlwaysOnTop(true);
    addChildComponent(text_entry_.get());

    setWantsKeyboardFocus(true);
    setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);



    setRotaryParameters(2.0f * electrosynth::kPi - kRotaryAngle, 2.0f * electrosynth::kPi + kRotaryAngle, true);




    setDefaultRange();

    setVelocityBasedMode(false);
    setVelocityModeParameters(1.0, 0, 0.0, false, juce::ModifierKeys::ctrlAltCommandModifiers);
}
//
//SynthSlider::SynthSlider(juce::String name, chowdsp::FloatParameter& param) : OpenGlSlider(name), show_popup_on_hover_(false), scroll_enabled_(true),
//                                        bipolar_modulation_(false), stereo_modulation_(false),
//                                        bypass_modulation_(false), modulation_bar_right_(true),
//                                        snap_to_value_(false), hovering_(false),
//                                        has_parameter_assignment_(false),
//                                        use_suffix_(true), snap_value_(0.0),
//                                        text_height_percentage_(0.0f),
//                                        sensitivity_(kDefaultSensitivity),
//                                        popup_placement_(juce::BubbleComponent::below),
//                                        modulation_control_placement_(juce::BubbleComponent::below),
//                                        max_display_characters_(kDefaultFormatLength),
//                                        max_decimal_places_(kDefaultFormatDecimalPlaces), shift_index_amount_(0),
//                                        shift_is_multiplicative_(false), mouse_wheel_index_movement_(1.0),
//                                        text_entry_width_percent_(kDefaultTextEntryWidthPercent),
//                                        text_entry_height_percent_(kDefaultTextEntryHeightPercent),
//                                        display_multiply_(0.0f), display_exponential_base_(2.0f),
//                                        string_lookup_(nullptr), extra_modulation_target_(nullptr),
//                                        synth_interface_(nullptr) /*attachment(param,pluginState, *this)*/{
//    //setAttachment(param, pluginState);
//    setComponentID (param.paramID);
//  text_entry_ = std::make_unique<OpenGlTextEditor>(name);
//  text_entry_->setMonospace();
//  text_entry_->setMultiLine(false);
//  text_entry_->setScrollToShowCursor(false);
//  text_entry_->addListener(this);
//  text_entry_->setSelectAllWhenFocused(true);
//  text_entry_->setKeyboardType(juce::TextEditor::numericKeyboard);
//  text_entry_->setJustification(juce::Justification::centred);
//  text_entry_->setAlwaysOnTop(true);
//  text_entry_->getImageComponent()->setAlwaysOnTop(true);
//  addChildComponent(text_entry_.get());
//
//  setWantsKeyboardFocus(true);
//  setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
//
//
//
//  setRotaryParameters(2.0f * electrosynth::kPi - kRotaryAngle, 2.0f * electrosynth::kPi + kRotaryAngle, true);
//
//
//
//
//  setDefaultRange();
//
//  setVelocityBasedMode(false);
//  setVelocityModeParameters(1.0, 0, 0.0, false, juce::ModifierKeys::ctrlAltCommandModifiers);
//}

PopupItems SynthSlider::createPopupMenu() {
  PopupItems options;

  if (isDoubleClickReturnEnabled())
    options.addItem(kDefaultValue, "Set to Default juce::Value");

  if (has_parameter_assignment_)
    options.addItem(kArmMidiLearn, "Learn MIDI Assignment");

//  if (has_parameter_assignment_ && synth_interface_->getSynth()->isMidiMapped(getName().toStdString()))
//    options.addItem(kClearMidiLearn, "Clear MIDI Assignment");

  options.addItem(kManualEntry, "Enter juce::Value");






  return options;
}

void SynthSlider::mouseDown(const juce::MouseEvent& e) {
//  SynthBase* synth = synth_interface_->getSynth();

  if (e.mods.isAltDown()) {
    showTextEntry();
    return;
  }

  if (e.mods.isPopupMenu()) {
    PopupItems options = createPopupMenu();
    parent_->showPopupSelector(this, e.getPosition(), options, [=](int selection) { handlePopupResult(selection); });
  }
  else {
    if (isRotary())
      setMouseDragSensitivity(kDefaultRotaryDragLength / sensitivity_);
    else {
      setSliderSnapsToMousePosition(false);
      setMouseDragSensitivity(std::max(getWidth(), getHeight()) / sensitivity_);
    }

    OpenGlSlider::mouseDown(e);
    //mainSynth->beginChangeGesture(getName().toStdString());

    for (SliderListener* listener : slider_listeners_)
      listener->mouseDown(this);

    showPopup(true);
  }
}

void SynthSlider::mouseDrag(const juce::MouseEvent& e) {

  
  float multiply = 1.0f;

    
  sensitive_mode_ = e.mods.isCommandDown();
  if (sensitive_mode_)
    multiply *= kSlowDragMultiplier;

  if (isRotary())
    setMouseDragSensitivity(kDefaultRotaryDragLength / (sensitivity_ * multiply));
  else {
    setSliderSnapsToMousePosition(false);
    setMouseDragSensitivity(std::max(getWidth(), getHeight()) / (sensitivity_ * multiply));
  }

  OpenGlSlider::mouseDrag(e);

  if (!e.mods.isPopupMenu())
    showPopup(true);
}

void SynthSlider::mouseUp(const juce::MouseEvent& e) {
  if (e.mods.isPopupMenu() || e.mods.isAltDown())
    return;

  //setDefaultRange();
  OpenGlSlider::mouseUp(e);

  for (SliderListener* listener : slider_listeners_)
    listener->mouseUp(this);

//  synth_interface_->getSynth()->endChangeGesture(getName().toStdString());
}

void SynthSlider::mouseEnter(const juce::MouseEvent &e) {
    OpenGlSlider::mouseEnter(e);

  for (SliderListener* listener : slider_listeners_)
    listener->hoverStarted(this);

  if (show_popup_on_hover_)
    showPopup(true);
  hovering_ = true;
  redoImage();
}

void SynthSlider::mouseExit(const juce::MouseEvent &e) {
  OpenGlSlider::mouseExit(e);
  for (SynthSlider::SliderListener* listener : slider_listeners_)
    listener->hoverEnded(this);

  hidePopup(true);
  hovering_ = false;
  redoImage();
}

void SynthSlider::mouseDoubleClick(const juce::MouseEvent& e) {
  OpenGlSlider::mouseDoubleClick(e);
  if (!e.mods.isPopupMenu()) {
    for (SliderListener* listener : slider_listeners_)
      listener->doubleClick(this);
  }
  showPopup(true);
}

void SynthSlider::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
  double interval = getInterval();
  if (scroll_enabled_ && !wheel.isSmooth && getInterval() > 0) {
    if (shift_index_amount_ && e.mods.isShiftDown()) {
      interval = shift_index_amount_;
      if (shift_is_multiplicative_) {
        if (wheel.deltaY > 0.0f)
          setValue(getValue() * interval * mouse_wheel_index_movement_);
        else
          setValue(getValue() / std::max(1.0, (interval * mouse_wheel_index_movement_)));
      }
    }

    if (wheel.deltaY > 0.0f)
      setValue(getValue() + interval * mouse_wheel_index_movement_);
    else
      setValue(getValue() - interval * mouse_wheel_index_movement_);
  }
  else
    OpenGlSlider::mouseWheelMove(e, wheel);

  showPopup(true);
}

void SynthSlider::focusLost(FocusChangeType cause) {
  for (SliderListener* listener : slider_listeners_)
    listener->focusLost(this);
}

void SynthSlider::valueChanged() {
  OpenGlSlider::valueChanged();
  notifyGuis();
}

juce::String SynthSlider::getRawTextFromValue(double value) {
  if (!has_parameter_assignment_)
    return OpenGlSlider::getTextFromValue(value);

  return juce::String(getAdjustedValue(value));
}

juce::String SynthSlider::getSliderTextFromValue(double value) {
//  if (string_lookup_) {
//    int lookup = electrosynth::utils::iclamp(value, 0, getMaximum());
//    return string_lookup_[lookup];
//  }

  //if (!has_parameter_assignment_)
    return OpenGlSlider::getTextFromValue(value);

  //double adjusted_value = getAdjustedValue(value);
 // return popup_prefix_ + formatValue(adjusted_value);
}

juce::String SynthSlider::getTextFromValue(double value) {
//  if (isText() && has_parameter_assignment_ && popup_prefix_.isEmpty()) {
//    if (details_.local_description.empty())
//      return details_.display_name;
//    return details_.local_description;
//  }
  if (isText() && !popup_prefix_.isEmpty())
    return popup_prefix_;
  return getSliderTextFromValue(value);
}

double SynthSlider::getValueFromText(const juce::String& text) {
  juce::String cleaned = text.removeCharacters(" ").toLowerCase();
  if (string_lookup_) {
    for (int i = 0; i <= getMaximum(); ++i) {
      if (cleaned == juce::String(string_lookup_[i]).toLowerCase())
        return i;
    }
  }
//  if (text.endsWithChar('%') && getDisplayDetails()->display_units != "%") {
//    float t = 0.01f * cleaned.removeCharacters("%").getDoubleValue();
//    return (getMaximum() - getMinimum()) * t + getMinimum();
//  }
  return getValueFromAdjusted(juce::Slider::getValueFromText(text));
}
double SynthSlider::getValueFromAdjusted(double value) {
  //vital::ValueDetails* details = getDisplayDetails();

  double readjusted_value = value;
  return value;
//  if (display_multiply_)
//    readjusted_value /= display_multiply_;
//  else
//    readjusted_value /= details->display_multiply;
//
//  if (details->display_invert)
//    readjusted_value = 1.0 / readjusted_value;
//  readjusted_value -= details->post_offset;
//
//  switch (details->value_scale) {
//    case vital::ValueDetails::kQuadratic:
//      return sqrtf(std::max(readjusted_value, 0.0));
//    case vital::ValueDetails::kCubic:
//      return powf(std::max(readjusted_value, 0.0), 1.0f / 3.0f);
//    case vital::ValueDetails::kQuartic:
//      return sqrtf(sqrtf(std::max(readjusted_value, 0.0)));
//    case vital::ValueDetails::kExponential:
//      return log(readjusted_value) / std::log(display_exponential_base_);
//    case vital::ValueDetails::kSquareRoot:
//      return readjusted_value * readjusted_value;
//    default:
//      return readjusted_value;
//  }
}
double SynthSlider::getAdjustedValue(double value) {
//  electrosynth::ValueDetails* details = getDisplayDetails();

  double adjusted_value = value;
  return value;
//  switch (details->value_scale) {
//    case electrosynth::ValueDetails::kQuadratic:
//      adjusted_value *= adjusted_value;
//      break;
//    case electrosynth::ValueDetails::kCubic:
//      adjusted_value *= adjusted_value * adjusted_value;
//      break;
//    case electrosynth::ValueDetails::kQuartic:
//      adjusted_value *= adjusted_value;
//      adjusted_value *= adjusted_value;
//      break;
//    case electrosynth::ValueDetails::kExponential:
//      adjusted_value = powf(display_exponential_base_, adjusted_value);
//      break;
//    case electrosynth::ValueDetails::kSquareRoot:
//      adjusted_value = sqrtf(std::max(adjusted_value, 0.0));
//      break;
//    default:
//      break;
//  }

//  adjusted_value += details->post_offset;
//  if (details->display_invert)
//    adjusted_value = 1.0 / adjusted_value;
//  if (display_multiply_)
//    adjusted_value *= display_multiply_;
//  else
//    adjusted_value *= details->display_multiply;
//
//  return adjusted_value;
}



void SynthSlider::setValueFromAdjusted(double value) {
  setValue(getValueFromAdjusted(value));
}

void SynthSlider::parentHierarchyChanged() {
  synth_interface_ = findParentComponentOfClass<SynthGuiInterface>();
  OpenGlSlider::parentHierarchyChanged();
}

double SynthSlider::snapValue(double attempted_value, DragMode drag_mode) {
  const double percent = 0.05;
  if (!snap_to_value_ || sensitive_mode_ || drag_mode != DragMode::absoluteDrag)
    return attempted_value;

  double range = getMaximum() - getMinimum();
  double radius = percent * range;
  if (attempted_value - snap_value_ <= radius && attempted_value - snap_value_ >= -radius)
    return snap_value_;
  return attempted_value;
}

void SynthSlider::textEditorReturnKeyPressed(juce::TextEditor& editor) {
  setSliderPositionFromText();
}

void SynthSlider::textEditorFocusLost(juce::TextEditor& editor) {
  setSliderPositionFromText();
}

void SynthSlider::setSliderPositionFromText() {
  if (text_entry_ && !text_entry_->getText().isEmpty())
    setValue(getValueFromText(text_entry_->getText()));
  text_entry_->setVisible(false);

  for (SliderListener* listener : slider_listeners_)
    listener->menuFinished(this);
}

void SynthSlider::showTextEntry() {
#if !defined(NO_TEXT_ENTRY)
  text_entry_->setColour(juce::CaretComponent::caretColourId, findColour(Skin::kTextEditorCaret, true));
  text_entry_->setColour(juce::TextEditor::textColourId, findColour(Skin::kBodyText, true));
  text_entry_->setColour(juce::TextEditor::highlightedTextColourId, findColour(Skin::kBodyText, true));
  text_entry_->setColour(juce::TextEditor::highlightColourId, findColour(Skin::kTextEditorSelection, true));
  if (isRotary())
    setRotaryTextEntryBounds();
  else
    setLinearTextEntryBounds();
  text_entry_->setVisible(true);

  text_entry_->redoImage();
  text_entry_->setText(getRawTextFromValue(getValue()));
  text_entry_->selectAll();
  if (text_entry_->isShowing())
    text_entry_->grabKeyboardFocus();
#endif
}

void SynthSlider::drawShadow(juce::Graphics &g) {
  if (isRotary() && !isTextOrCurve())
    drawRotaryShadow(g);
  else if (&getLookAndFeel() == CurveLookAndFeel::instance()) {
    g.setColour(findColour(Skin::kWidgetBackground, true));
    float rounding = findValue(Skin::kWidgetRoundedCorner);
    g.fillRoundedRectangle(getBounds().toFloat(), rounding);
  }
}

void SynthSlider::drawRotaryShadow(juce::Graphics &g) {
    juce::Colour shadow_color = findColour(Skin::kShadow, true);
//    DBG("drawshadow ----" + getName() + "-----");
//
//
//    DBG("component x" + juce::String(getX()));
//    DBG("componoent y" + juce::String(getY()));
   // DBG("scale" + juce::String(parent_->getDisplayScale()));
    int width = getWidth(); //*parent_->getDisplayScale();
    int height = getHeight();// *parent_->getDisplayScale();
    int x = getX();// * parent_->getDisplayScale();
    int y = getY();// * parent_->getDisplayScale();
    float center_x = (float)width / 2.0f;
    float center_y = (float)height / 2.0f;
    float stroke_width = findValue(Skin::kKnobArcThickness);
    float radius =   knob_size_scale_ * findValue(Skin::kKnobArcSize) / 2.0f;
    center_y += findValue(Skin::kKnobOffset);
    float shadow_width = findValue(Skin::kKnobShadowWidth);
    float shadow_offset = findValue(Skin::kKnobShadowOffset);

    juce::PathStrokeType outer_stroke(stroke_width, juce::PathStrokeType::beveled, juce::PathStrokeType::rounded);
    juce::PathStrokeType shadow_stroke(stroke_width + 1, juce::PathStrokeType::beveled, juce::PathStrokeType::rounded);
    g.saveState();
    //g.setColour(juce::Colours::white);
    //g.fillRect(x, y, width, height);
    g.setOrigin(x, y);
    juce::Colour body = findColour(Skin::kRotaryBody, true);
    float body_radius =  knob_size_scale_ * findValue(Skin::kKnobBodySize) / 2.0f;
    if (body_radius >= 0.0f && body_radius < width) {

//        if (shadow_width > 0.0f) {
//            juce::Colour transparent_shadow = shadow_color.withAlpha(0.0f);
//            float shadow_radius = body_radius + shadow_width;
//            juce::ColourGradient shadow_gradient(shadow_color, center_x, center_y + shadow_offset,
//                                                 transparent_shadow, center_x - shadow_radius, center_y + shadow_offset, true);
//            float shadow_start = std::max(0.0f, (body_radius - std::abs(shadow_offset))) / shadow_radius;
//            shadow_gradient.addColour(shadow_start, shadow_color);
//            shadow_gradient.addColour(1.0f - (1.0f - shadow_start) * 0.75f, shadow_color.withMultipliedAlpha(0.5625f));
//            shadow_gradient.addColour(1.0f - (1.0f - shadow_start) * 0.5f, shadow_color.withMultipliedAlpha(0.25f));
//            shadow_gradient.addColour(1.0f - (1.0f - shadow_start) * 0.25f, shadow_color.withMultipliedAlpha(0.0625f));
//            g.setGradientFill(shadow_gradient);
//            g.fillRect(getLocalBounds());
//        }

        g.setColour(body);
        juce::Rectangle<float> ellipse(center_x - body_radius, center_y - body_radius, 2.0f * body_radius, 2.0f * body_radius);
        g.fillEllipse(ellipse);

        g.setColour(findColour(Skin::kRotaryBodyBorder, true));
        g.drawEllipse(ellipse.reduced(0.5f), 1.0f);
    }

    juce::Path shadow_outline;
    juce::Path shadow_path;

    shadow_outline.addCentredArc(center_x, center_y, radius, radius,
                                 0, -kRotaryAngle, kRotaryAngle, true);
    shadow_stroke.createStrokedPath(shadow_path, shadow_outline);
    if ((!findColour(Skin::kRotaryArcUnselected, true).isTransparent() && isActive()) ||
        (!findColour(Skin::kRotaryArcUnselectedDisabled, true).isTransparent() && !isActive())) {
        g.setColour(shadow_color);
        g.fillPath(shadow_path);
    }

    g.restoreState();
}

void SynthSlider::setDefaultRange() {
  if (!has_parameter_assignment_)
    return;

//  if (details_.value_scale == electrosynth::ValueDetails::kIndexed)
//    setRange(details_.min, details_.max, 1.0f);
//  else
//    setRange(details_.min, details_.max);
}

void SynthSlider::addSliderListener(SynthSlider::SliderListener* listener) {
  slider_listeners_.push_back(listener);
}

void SynthSlider::showPopup(bool primary) {
  if (shouldShowPopup())
    parent_->showPopupDisplay(this, getTextFromValue(getValue()).toStdString(), popup_placement_, primary);
}

void SynthSlider::hidePopup(bool primary) {
  parent_->hidePopupDisplay(primary);
}

juce::String SynthSlider::formatValue(float value) {
  juce::String format;
//  if (details_.value_scale == electrosynth::ValueDetails::kIndexed)
//    format = juce::String(value);
//  else {
//    if (max_decimal_places_ == 0)
//      format = juce::String(std::roundf(value));
//    else
//      format = juce::String(value, max_decimal_places_);
//
//    int display_characters = max_display_characters_;
//    if (format[0] == '-')
//      display_characters += 1;
//
//    format = format.substring(0, display_characters);
//    if (format.getLastCharacter() == '.')
//      format = format.removeCharacters(".");
//  }
//
//  if (use_suffix_)
//    return format + getDisplayDetails()->display_units;
  return format;
}

void SynthSlider::notifyGuis() {
//  for (SynthSlider::SliderListener* listener : slider_listeners_)
//    listener->guiChanged(this);
}

juce::Rectangle<int> SynthSlider::getModulationMeterBounds() const {
  static constexpr int kTextBarSize = 2;

  juce::Rectangle<int> mod_bounds = getModulationArea();
  if (isTextOrCurve()) {
    if (modulation_bar_right_)
      return mod_bounds.removeFromRight(kTextBarSize);

    return mod_bounds.removeFromLeft(kTextBarSize);
  }
  else if (isRotary())
    return getLocalBounds();

  int buffer = findValue(Skin::kWidgetMargin);
  if (getSliderStyle() == LinearBar) {
    return juce::Rectangle<int>(mod_bounds.getX() + buffer, mod_bounds.getY(),
                          mod_bounds.getWidth() - 2 * buffer, mod_bounds.getHeight());
  }
  return juce::Rectangle<int>(mod_bounds.getX(), mod_bounds.getY() + buffer,
                        mod_bounds.getWidth(), mod_bounds.getHeight() - 2 * buffer);
}



void SynthSlider::handlePopupResult(int result) {
  //SynthBase* synth = synth_interface_->getSynth();


//  if (result == kArmMidiLearn)
//    mainSynth->armMidiLearn(getName().toStdString());
//  else if (result == kClearMidiLearn)
//    mainSynth->clearMidiLearn(getName().toStdString());
//  else if (result == kDefaultValue)
//    setValue(getDoubleClickReturnValue());
//  else if (result == kManualEntry)
//    showTextEntry();
//  else if (result == kClearModulations) {
//    for (electrosynth::ModulationConnection* connection : connections) {
//      std::string source = connection->source_name;
//      synth_interface_->disconnectModulation(connection);
//    }
//    notifyModulationsChanged();
//  }
//  else if (result >= kModulationList) {
//    int connection_index = result - kModulationList;
//    std::string source = connections[connection_index]->source_name;
//    synth_interface_->disconnectModulation(connections[connection_index]);
//    notifyModulationsChanged();
//  }
}

void SynthSlider::setRotaryTextEntryBounds() {
  int text_width = getWidth() * text_entry_width_percent_;
  float font_size = findValue(Skin::kTextComponentFontSize);
  int text_height = font_size / kTextEntryHeightPercent;
  float y_offset = 0.0f;
  if (isText())
    y_offset = findValue(Skin::kTextComponentOffset);

  text_entry_->setBounds((getWidth() - text_width) / 2, (getHeight() - text_height + 1) / 2 + y_offset,
                         text_width, text_height);
}

void SynthSlider::setLinearTextEntryBounds() {
  static constexpr float kTextEntryWidthRatio = 3.0f;

  float font_size = findValue(Skin::kTextComponentFontSize);
  int text_height = font_size / kTextEntryHeightPercent;
  int text_width = text_height * kTextEntryWidthRatio;

  text_entry_->setBounds((getWidth() - text_width) / 2, (getHeight() - text_height) / 2, text_width, text_height);
}

void SynthSlider::notifyModulationAmountChanged() {
  for (SynthSlider::SliderListener* listener : slider_listeners_)
    listener->modulationAmountChanged(this);
}

void SynthSlider::notifyModulationRemoved() {
  for (SynthSlider::SliderListener* listener : slider_listeners_)
    listener->modulationRemoved(this);
}

void SynthSlider::notifyModulationsChanged() {
  for (SynthSlider::SliderListener* listener : slider_listeners_)
    listener->modulationsChanged(getName().toStdString());
}
