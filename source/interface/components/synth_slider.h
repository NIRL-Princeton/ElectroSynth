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

#pragma once

#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "curve_look_and_feel.h"
#include "text_look_and_feel.h"
#include "open_gl_image_component.h"
#include "open_gl_multi_quad.h"
#include "synth_section.h"

class FullInterface;
class OpenGlSlider;
class SynthGuiInterface;
class SynthSection;

class OpenGlSliderQuad : public OpenGlQuad {
  public:
    OpenGlSliderQuad(OpenGlSlider* slider,juce::String name) : OpenGlQuad(Shaders::kRotarySliderFragment, name + "_quad"), slider_(slider) { }
    virtual void init(OpenGlWrapper& open_gl) override;

    void paintBackground(juce::Graphics& g) override;

  private:
    OpenGlSlider* slider_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlSliderQuad)
};

class OpenGlSlider : public juce::Slider {
  public:
    static constexpr float kRotaryAngle = 0.8f * electrosynth::kPi;

    OpenGlSlider(juce::String name) : juce::Slider(name), parent_(nullptr), modulation_knob_(false), modulation_amount_(0.0f),
                                paint_to_image_(false), active_(true), bipolar_(false), slider_quad_(new OpenGlSliderQuad(this,name)),
                                image_component_(new OpenGlImageComponent(name)),
                                knob_size_scale_(1.0f) {
      slider_quad_->setTargetComponent(this);
      setMaxArc(kRotaryAngle);

      image_component_->paintEntireComponent(false);
      image_component_->setComponent(this);
      image_component_->setScissor(true);

      slider_quad_->setActive(false);
      image_component_->setActive(false);

    }

    virtual void resized() override {
        juce::Slider::resized();
        //image_component_.setBounds(getLocalBounds());
      setColors();
      setSliderDisplayValues();
    }
      
    virtual void valueChanged() override {
      juce::Slider::valueChanged();
      redoImage();
    }
    virtual void paint(juce::Graphics &g) override {}
//    virtual  void  paint(juce::Graphics &g) override
//    {
//        juce::Colour shadow_color = findColour(Skin::kShadow, true);
//
//        float center_x = getWidth() / 2.0f;
//        float center_y = getHeight() / 2.0f;
//        float stroke_width = findValue(Skin::kKnobArcThickness);
//        float radius = knob_size_scale_ * findValue(Skin::kKnobArcSize) / 2.0f;
//        //center_y += findValue(Skin::kKnobOffset);
//        float shadow_width = findValue(Skin::kKnobShadowWidth);
//        float shadow_offset = findValue(Skin::kKnobShadowOffset);
//
//        juce::PathStrokeType outer_stroke(stroke_width, juce::PathStrokeType::beveled, juce::PathStrokeType::rounded);
//        juce::PathStrokeType shadow_stroke(stroke_width + 1, juce::PathStrokeType::beveled, juce::PathStrokeType::rounded);
//
//        g.saveState();
////        g.fillAll(juce::Colours::white);
////        g.setOrigin(center_juce::x,center_y);
////
////        juce::Colour body = findColour(Skin::kRotaryBody, true);
////        float body_radius = knob_size_scale_ * findValue(Skin::kKnobBodySize) / 2.0f;
////        if (body_radius >= 0.0f && body_radius < getWidth()) {
////
////            if (shadow_width > 0.0f) {
////                juce::Colour transparent_shadow = shadow_color.withAlpha(0.0f);
////                float shadow_radius = body_radius + shadow_width;
////                juce::ColourGradient shadow_gradient(shadow_color, center_x, center_y + shadow_offset,
////                                               transparent_shadow, center_x - shadow_radius, center_y + shadow_offset, true);
////                float shadow_start = std::max(0.0f, (body_radius - std::abs(shadow_offset))) / shadow_radius;
////                shadow_gradient.addColour(shadow_start, shadow_color);
////                shadow_gradient.addColour(1.0f - (1.0f - shadow_start) * 0.75f, shadow_color.withMultipliedAlpha(0.5625f));
////                shadow_gradient.addColour(1.0f - (1.0f - shadow_start) * 0.5f, shadow_color.withMultipliedAlpha(0.25f));
////                shadow_gradient.addColour(1.0f - (1.0f - shadow_start) * 0.25f, shadow_color.withMultipliedAlpha(0.0625f));
////                g.setGradientFill(shadow_gradient);
////                g.fillRect(getLocalBounds());
////            }
////
////            g.setColour(body);
//////            DBG("x: " + juce::String(center_x- body_radius));
//////            DBG("y: " + juce::String(center_y- body_radius));
//////            DBG("body radius: " + juce::String(body_radius));
//////            DBG("knob size scale " + juce::String(knob_size_scale_));
//////            DBG("knob body size" + juce::String(findValue(Skin::kKnobBodySize)));
////            juce::Rectangle<float> ellipse(10 - 1.5f*body_radius ,10 - 1.5f*body_radius, 1.5f*body_radius,1.5f*body_radius);
////            //ellipse.setCentre(getWidth() / 2.0f, getHeight()/2);
////            //juce::Rectangle<float> ellipse(0, 0,  2.0f * body_radius, 2.0f * body_radius);
////
////            g.fillEllipse(ellipse);
////
////            g.setColour(findColour(Skin::kRotaryBodyBorder, true));
////            g.drawEllipse(ellipse.reduced(0.5f), 1.0f);
////        }
////
////        juce::Path shadow_outline;
////        juce::Path shadow_path;
////
////        shadow_outline.addCentredArc(0, 0, radius, radius,
////                                     0, -kRotaryAngle, kRotaryAngle, true);
////        shadow_stroke.createStrokedPath(shadow_path, shadow_outline);
////        if ((!findColour(Skin::kRotaryArcUnselected, true).isTransparent() && isActive()) ||
////            (!findColour(Skin::kRotaryArcUnselectedDisabled, true).isTransparent() && !isActive())) {
////            g.setColour(shadow_color);
////            g.fillPath(shadow_path);
////        }
//
//        g.restoreState();
//        //DBG("paint");
//    }

//    void init (OpenGlWrapper &open_gl) override
//    {
//
//    }

    void parentHierarchyChanged() override {
      parent_ = findParentComponentOfClass<SynthSection>();
      juce::Slider::parentHierarchyChanged();
    }

    void paintToImage(bool paint) {
      paint_to_image_ = paint;
    }

    bool isText() const {
      return &getLookAndFeel() == TextLookAndFeel::instance();
    }

    bool isTextOrCurve() const {
      return isText() || &getLookAndFeel() == CurveLookAndFeel::instance();
    }

    bool isModulationKnob() const {
      return modulation_knob_;
    }

    bool isRotaryQuad() const {
      return !paint_to_image_ && getSliderStyle() == RotaryHorizontalVerticalDrag && !isTextOrCurve();
    }

    bool isHorizontalQuad() const {
      return !paint_to_image_ && getSliderStyle() == LinearBar && !isTextOrCurve();
    }

    bool isVerticalQuad() const {
      return !paint_to_image_ && getSliderStyle() == LinearBarVertical && !isTextOrCurve();
    }

    std::shared_ptr<OpenGlComponent> getImageComponent() {
      return image_component_;
    }

    std::shared_ptr<OpenGlComponent> getQuadComponent() {
      return slider_quad_;
    }

    void setMaxArc(float arc) {
      slider_quad_->setMaxArc(arc);
    }

    void setModulationKnob() { modulation_knob_ = true; }
    void setModulationAmount(float modulation) { modulation_amount_ = modulation; redoImage(); }
    float getModulationAmount() const { return modulation_amount_; }

    virtual float getKnobSizeScale() const { return 1.0f; }
    bool isBipolar() const { return bipolar_; }
    bool isActive() const { return active_; }

    void setBipolar(bool bipolar = true) {
      if (bipolar_ == bipolar)
        return;
      
      bipolar_ = bipolar;
      redoImage();
    }

    void setActive(bool active = true) {
      if (active_ == active)
        return;

      active_ = active;
      setColors();
      redoImage();
    }

    virtual juce::Colour getModColor() const {
      return findColour(Skin::kModulationMeterControl, true);
    }

    virtual juce::Colour getBackgroundColor() const {
      return findColour(Skin::kWidgetBackground, true);
    }

    virtual juce::Colour getUnselectedColor() const {
      if (isModulationKnob())
        return findColour(Skin::kWidgetBackground, true);
      if (isRotary()) {
        if (active_)
          return findColour(Skin::kRotaryArcUnselected, true);
        return findColour(Skin::kRotaryArcUnselectedDisabled, true);
      }

      return findColour(Skin::kLinearSliderUnselected, true);
    }

    virtual juce::Colour getSelectedColor() const {
      if (isModulationKnob()) {
        juce::Colour background = findColour(Skin::kWidgetBackground, true);
        if (active_)
          return findColour(Skin::kRotaryArc, true).interpolatedWith(background, 0.5f);
        return findColour(Skin::kRotaryArcDisabled, true).interpolatedWith(background, 0.5f);
      }
      if (isRotary()) {
        if (active_)
          return findColour(Skin::kRotaryArc, true);
        return findColour(Skin::kRotaryArcDisabled, true);
      }
      if (active_)
        return findColour(Skin::kLinearSlider, true);
      return findColour(Skin::kLinearSliderDisabled, true);
    }

    virtual juce::Colour getThumbColor() const {
      if (isModulationKnob())
        return findColour(Skin::kRotaryArc, true);
      if (isRotary())
        return findColour(Skin::kRotaryHand, true);
      if (active_)
        return findColour(Skin::kLinearSliderThumb, true);
      return findColour(Skin::kLinearSliderThumbDisabled, true);
    }

    int getLinearSliderWidth();
    void setSliderDisplayValues();
    void redoImage(bool skip_image = false);
    void setColors();

    virtual float findValue(Skin::ValueId value_id) const {
      if (parent_)
        return parent_->findValue(value_id);
      return 0.0f;
    }

    void setAlpha(float alpha, bool reset = false) { slider_quad_->setAlpha(alpha, reset); }
    void setDrawWhenNotVisible(bool draw) { slider_quad_->setDrawWhenNotVisible(draw); }

    SynthSection* getSectionParent() { return parent_; }

  protected:
    SynthSection* parent_;
    float knob_size_scale_;
  private:
    juce::Colour thumb_color_;
    juce::Colour selected_color_;
    juce::Colour unselected_color_;
    juce::Colour background_color_;
    juce::Colour mod_color_;

    bool modulation_knob_;
    float modulation_amount_;
    bool paint_to_image_;
    bool active_;
    bool bipolar_;

    std::shared_ptr<OpenGlSliderQuad> slider_quad_;
    std::shared_ptr<OpenGlImageComponent> image_component_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlSlider)
};

class SynthSlider : public OpenGlSlider, public juce::TextEditor::Listener {
  public:
    enum MenuId {
      kCancel = 0,
      kArmMidiLearn,
      kClearMidiLearn,
      kDefaultValue,
      kManualEntry,
      kClearModulations,
      kModulationList
    };

    static constexpr int kDefaultFormatLength = 5;
    static constexpr int kDefaultFormatDecimalPlaces = 5;
    static constexpr float kDefaultTextEntryWidthPercent = 0.9f;
    static constexpr float kDefaultTextEntryHeightPercent = 0.35f;
    static constexpr float kModulationSensitivity = 0.003f;
    static constexpr float kTextEntryHeightPercent = 0.6f;

    static constexpr float kSlowDragMultiplier = 0.1f;
    static constexpr float kDefaultSensitivity = 1.0f;

    static constexpr float kDefaultTextHeightPercentage = 0.7f;
    static constexpr float kDefaultRotaryDragLength = 200.0f;
    static constexpr float kRotaryModulationControlPercent = 0.26f;

    static constexpr float kLinearWidthPercent = 0.26f;
    static constexpr float kLinearHandlePercent = 1.2f;
    static constexpr float kLinearModulationPercent = 0.1f;

    class SliderListener {
      public:
        virtual ~SliderListener() { }
        virtual void hoverStarted(SynthSlider* slider) { }
        virtual void hoverEnded(SynthSlider* slider) { }
        virtual void mouseDown(SynthSlider* slider) { }
        virtual void mouseUp(SynthSlider* slider) { }
        virtual void beginModulationEdit(SynthSlider* slider) { }
        virtual void endModulationEdit(SynthSlider* slider) { }
        virtual void menuFinished(SynthSlider* slider) { }
        virtual void focusLost(SynthSlider* slider) { }
        virtual void doubleClick(SynthSlider* slider) { }
        virtual void modulationsChanged(const std::string& name) { }
        virtual void modulationAmountChanged(SynthSlider* slider) { }
        virtual void modulationRemoved(SynthSlider* slider) { }
        virtual void guiChanged(SynthSlider* slider) { }
    };

    //SynthSlider(juce::String name, chowdsp::FloatParameter& param);
    SynthSlider(juce::String name);
    virtual void mouseDown(const juce::MouseEvent& e) override;
    virtual void mouseDrag(const juce::MouseEvent& e) override;
    virtual void mouseEnter(const juce::MouseEvent& e) override;
    virtual void mouseExit(const juce::MouseEvent& e) override;
    virtual void mouseUp(const juce::MouseEvent& e) override;
    virtual void mouseDoubleClick(const juce::MouseEvent& e) override;
    virtual void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    virtual void focusLost(FocusChangeType cause) override;

    virtual void valueChanged() override;
    juce::String getRawTextFromValue(double value);
    juce::String getSliderTextFromValue(double value);
    juce::String getTextFromValue(double value) override;
    double getValueFromText(const juce::String& text) override;
    double getAdjustedValue(double value);
    double getValueFromAdjusted(double value);
    void setValueFromAdjusted(double value);
    virtual void parentHierarchyChanged() override;

    virtual double snapValue(double attemptedValue, DragMode dragMode) override;

    void textEditorTextChanged(juce::TextEditor&) override {
      text_entry_->redoImage();
    }
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;
    void setSliderPositionFromText();

    void showTextEntry();
    virtual bool shouldShowPopup() { return true; }

    virtual void drawShadow(juce::Graphics& g);
    void drawRotaryShadow(juce::Graphics& g);
    void snapToValue(bool snap, float value = 0.0) {
      snap_to_value_ = snap;
      snap_value_ = value;
    }
//
//    void setScalingType(vital::ValueDetails::ValueScale scaling_type) {
//      details_.value_scale = scaling_type;
//    }
//
//    vital::ValueDetails::ValueScale getScalingType() const { return details_.value_scale; }

    void setStringLookup(const std::string* lookup) {
      string_lookup_ = lookup;
    }
    void setScrollEnabled(bool enabled) {
      scroll_enabled_ = enabled;
      setScrollWheelEnabled(enabled);
    }
    const std::string* getStringLookup() const { return string_lookup_; }


    juce::String formatValue(float value);

    void setDefaultRange();

    void addSliderListener(SliderListener* listener);

    void showPopup(bool primary);
    void hidePopup(bool primary);
    void setPopupPlacement(juce::BubbleComponent::BubblePlacement placement) {
      popup_placement_ = placement;
    }
    void setModulationPlacement(juce::BubbleComponent::BubblePlacement placement) {
      modulation_control_placement_ = placement;
    }
    juce::BubbleComponent::BubblePlacement getPopupPlacement() { return popup_placement_; }
    juce::BubbleComponent::BubblePlacement getModulationPlacement() { return modulation_control_placement_; }

    void notifyGuis();
    void handlePopupResult(int result);

    void setSensitivity(double sensitivity) { sensitivity_ = sensitivity; }
    double getSensitivity() { return sensitivity_; }

    juce::Rectangle<int> getModulationMeterBounds() const;
    bool hasModulationArea() const {
      return modulation_area_.getWidth();
    }
    juce::Rectangle<int> getModulationArea() const {
      if (modulation_area_.getWidth())
        return modulation_area_;
      return getLocalBounds();
    }
    void setModulationArea(juce::Rectangle<int> area) { modulation_area_ = area; }
    bool isModulationBipolar() const { return bipolar_modulation_; }
    bool isModulationStereo() const { return stereo_modulation_; }
    bool isModulationBypassed() const { return bypass_modulation_; }

    void setTextHeightPercentage(float percentage) { text_height_percentage_ = percentage; }
    float getTextHeightPercentage() { return text_height_percentage_; }
    float mouseHovering() const { return hovering_; }

    void setMouseWheelMovement(double movement) { mouse_wheel_index_movement_ = movement; }

    void setMaxDisplayCharacters(int characters) { max_display_characters_ = characters; }
    void setMaxDecimalPlaces(int decimal_places) { max_decimal_places_ = decimal_places; }
    void setTextEntrySizePercent(float width_percent, float height_percent) {
      text_entry_width_percent_ = width_percent;
      text_entry_height_percent_ = height_percent;
      redoImage();
    }
    void setTextEntryWidthPercent(float percent) { text_entry_height_percent_ = percent; redoImage(); }
    void setShiftIndexAmount(int shift_amount) { shift_index_amount_ = shift_amount; }
    void setShowPopupOnHover(bool show) { show_popup_on_hover_ = show; }
    void setPopupPrefix(juce::String prefix) { popup_prefix_ = prefix; }
    void setKnobSizeScale(float scale) { knob_size_scale_ = scale; }
    float getKnobSizeScale() const override { return knob_size_scale_; }
    void useSuffix(bool use) { use_suffix_ = use; }
    void setExtraModulationTarget(juce::Component* component) { extra_modulation_target_ = component; }
    juce::Component* getExtraModulationTarget() { return extra_modulation_target_; }
    void setModulationBarRight(bool right) { modulation_bar_right_ = right; }
    bool isModulationBarRight() { return modulation_bar_right_; }

    void setDisplayMultiply(float multiply) { display_multiply_ = multiply; }
    void setDisplayExponentialBase(float base) { display_exponential_base_ = base; }

    void overrideValue(Skin::ValueId value_id, float value) {
      value_lookup_[value_id] = value;
    }

    float findValue(Skin::ValueId value_id) const override {
      if (value_lookup_.count(value_id))
        return value_lookup_.at(value_id);
      return OpenGlSlider::findValue(value_id);
    }





    std::shared_ptr<OpenGlComponent> getTextEditorComponent() { return text_entry_->getImageComponent(); }

//    void setAttachment(chowdsp::FloatParameter& param, chowdsp::PluginState& pluginState)
//    {
//        attachment = (param, pluginState, *this);
//    }
  protected:
    chowdsp::SliderAttachment attachment;
    PopupItems createPopupMenu();
    void setRotaryTextEntryBounds();
    void setLinearTextEntryBounds();
    void notifyModulationAmountChanged();
    void notifyModulationRemoved();
    void notifyModulationsChanged();

    bool show_popup_on_hover_;
    juce::String popup_prefix_;
    bool scroll_enabled_;
    bool bipolar_modulation_;
    bool stereo_modulation_;
    bool bypass_modulation_;
    bool modulation_bar_right_;
    juce::Rectangle<int> modulation_area_;
    bool sensitive_mode_;
    bool snap_to_value_;
    bool hovering_;
    bool has_parameter_assignment_;
    bool use_suffix_;
    float snap_value_;
    float text_height_percentage_;
    //float knob_size_scale_;
    double sensitivity_;
    juce::BubbleComponent::BubblePlacement popup_placement_;
    juce::BubbleComponent::BubblePlacement modulation_control_placement_;
    int max_display_characters_;
    int max_decimal_places_;
    int shift_index_amount_;
    bool shift_is_multiplicative_;
    double mouse_wheel_index_movement_;
    float text_entry_width_percent_;
    float text_entry_height_percent_;

    std::map<Skin::ValueId, float> value_lookup_;
    juce::Point<int> last_modulation_edit_position_;
    juce::Point<int> mouse_down_position_;


    float display_multiply_;
    float display_exponential_base_;



    const std::string* string_lookup_;

    juce::Component* extra_modulation_target_;
    SynthGuiInterface* synth_interface_;
    std::unique_ptr<OpenGlTextEditor> text_entry_;

    std::vector<SliderListener*> slider_listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthSlider)
};

