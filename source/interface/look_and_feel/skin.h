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


#include <juce_gui_basics/juce_gui_basics.h>

#include "json.hpp"
using json = nlohmann::json;
class FullInterface;
class SynthSection;

class Skin {
  public:
    enum SectionOverride {
      kNone,
      kLogo,
      kHeader,
      kOverlay,
      kPopupBrowser,
      kPresetBrowser,
          kModulationDragDrop,
          kModulationSection,
      kNumSectionOverrides
    };

    // defines sizing/geometry values
    enum ValueId {
        kBodyRounding,
        kLabelHeight,
        kLabelBackgroundHeight,
        kLabelBackgroundRounding,
        kLabelOffset,
        kTextComponentLabelOffset,
        kRotaryOptionXOffset,
        kRotaryOptionYOffset,
        kRotaryOptionWidth,
        kTitleWidth,
        kPadding,
        kLargePadding,
        kSliderWidth,
        kTextComponentHeight,
        kTextComponentOffset,
        kTextComponentFontSize,
        kTextButtonHeight,
        kButtonFontSize,
        kKnobArcSize,
        kKnobArcThickness,
        kKnobBodySize,
        kKnobHandleLength,
        kKnobModAmountArcSize,
        kKnobModAmountArcThickness,
        kKnobModMeterArcSize,
        kKnobModMeterArcThickness,
        kKnobOffset,
        kKnobSectionHeight,
        kKnobShadowWidth,
        kKnobShadowOffset,
        kModulationButtonWidth,
        kModulationFontSize,
        kWidgetMargin,
        kWidgetRoundedCorner,
        kWidgetLineWidth,
        kWidgetLineBoost,
        kWidgetFillCenter,
        kWidgetFillFade,
        kWidgetFillBoost,
        kWavetableHorizontalAngle,
        kWavetableVerticalAngle,
        kWavetableDrawWidth,
        kWavetableWaveHeight,
        kWavetableYOffset,
        kNumSkinValueIds,
        kFrequencyDisplay = kNumSkinValueIds,
        kNumAllValueIds,
    };

    // defines every named color in the GUI
    enum ColorId {
        kInitialColor = 0x42345678,
        kBackground = kInitialColor,
        kBody,
        kBodyHeading,
        kHeadingText,
        kPresetText,
        kBodyText,
        kBorder,
        kLabelBackground,
        kLabelConnection,
        kPowerButtonOn,
        kPowerButtonOff,

        kOverlayScreen,
        kLightenScreen,
        kShadow,
        kPopupSelectorBackground,
        kPopupBackground,
        kPopupBorder,

        kTextComponentBackground,
        kTextComponentText,

        kRotaryArc,
        kRotaryArcDisabled,
        kRotaryArcUnselected,
        kRotaryArcUnselectedDisabled,
        kRotaryHand,
        kRotaryBody,
        kRotaryBodyBorder,

        kLinearSlider,
        kLinearSliderDisabled,
        kLinearSliderUnselected,
        kLinearSliderThumb,
        kLinearSliderThumbDisabled,

        kWidgetCenterLine,
        kWidgetPrimary1,
        kWidgetPrimary2,
        kWidgetPrimaryDisabled,
        kWidgetSecondary1,
        kWidgetSecondary2,
        kWidgetSecondaryDisabled,
        kWidgetAccent1,
        kWidgetAccent2,
        kWidgetBackground,

        kModulationMeter,
        kModulationMeterLeft,
        kModulationMeterRight,
        kModulationMeterControl,
        kModulationButtonSelected,
        kModulationButtonDragging,
        kModulationButtonUnselected,

        kIconSelectorIcon,

        kIconButtonOff,
        kIconButtonOffHover,
        kIconButtonOffPressed,
        kIconButtonOn,
        kIconButtonOnHover,
        kIconButtonOnPressed,

        kUiButton,
        kUiButtonText,
        kUiButtonHover,
        kUiButtonPressed,
        kUiActionButton,
        kUiActionButtonHover,
        kUiActionButtonPressed,

        kTextEditorBackground,
        kTextEditorBorder,
        kTextEditorCaret,
        kTextEditorSelection,

        kFinalColor
    };

    static constexpr int kNumColors = kFinalColor - kInitialColor;
    static bool shouldScaleValue(ValueId value_id);

    Skin();

    void setComponentColors(juce::Component* component) const;
    void setComponentColors(juce::Component* component, SectionOverride section_override, bool top_level = false) const;
    void setComponentValues(SynthSection* component) const;
    void setComponentValues(SynthSection* component, SectionOverride section_override, bool top_level = false) const;
    void setColor(ColorId color_id, const juce::Colour& color) { colors_[color_id - kInitialColor] = color; }
    juce::Colour getColor(ColorId color_id) const { return colors_[color_id - kInitialColor]; }
    juce::Colour getColor(int section, ColorId color_id) const;
    bool overridesColor(int section, ColorId color_id) const;
    bool overridesValue(int section, ValueId value_id) const;
    void copyValuesToLookAndFeel(juce::LookAndFeel* look_and_feel) const;

    void setValue(ValueId value_id, float value) { values_[value_id] = value; }
    float getValue(ValueId value_id) const { return values_[value_id]; }
    float getValue(int section, ValueId value_id) const;

    void addOverrideColor(int section, ColorId color_id, juce::Colour color);
    void removeOverrideColor(int section, ColorId color_id);

    void addOverrideValue(int section, ValueId value_id, float value);
    void removeOverrideValue(int section, ValueId value_id);

    json stateToJson();
    juce::String stateToString();
    void saveToFile(juce::File destination);

    json updateJson(json data);
    void jsonToState(json skin_var);
    bool stringToState(juce::String skin_string);
    bool loadFromFile(juce::File source);
    void loadDefaultSkin();
    void clearSkin();

  protected:
    juce::Colour colors_[kNumColors];
    float values_[kNumSkinValueIds];
    std::map<ColorId, juce::Colour> color_overrides_[kNumSectionOverrides];
    std::map<ValueId, float> value_overrides_[kNumSectionOverrides];
};

class SkinDesigner : public juce::DocumentWindow {
  public:
    SkinDesigner(Skin* skin, FullInterface* full_interface);
    ~SkinDesigner();

    void closeButtonPressed() override {
      delete this;
    }

  protected:
    std::unique_ptr<Component> container_;
};

