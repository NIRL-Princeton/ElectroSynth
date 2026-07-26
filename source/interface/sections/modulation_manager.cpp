/* Copyright 2013-2019 Matt Tytel
 *
 * electrosynth is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * electrosynth is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with electrosynth.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "modulation_manager.h"

#include "FullInterface.h"
#include "ModulationConnection.h"
#include "ParameterView/ParametersView.h"
#include "bar_renderer.h"
#include "midi_manager.h"
#include "modulation_meter.h"
#include "paths.h"
#include "shaders.h"
#include "skin.h"
#include "synth_base.h"
#include "synth_gui_interface.h"
#include <cmath>
#include <juce_gui_basics/juce_gui_basics.h>

namespace {
    constexpr float kDefaultModulationRatio = 0.0f; // default to 25% modulation upon making a new connection
    constexpr float kModSmoothDecay = 0.5f; // smoothing speed for modulation value animation/readout updates

    // recursively checks if a component and all its parents are visible before showing modulation on knobs
    bool allVisible(juce::Component* component) {
        if (component == nullptr || component->getParentComponent() == nullptr)
            return true;
        return component->isVisible() && allVisible(component->getParentComponent());
  }

  juce::String getModulationSourceLabel(const juce::String& source_name) {
    juce::String prefix;
    if (source_name.startsWithIgnoreCase("env"))
      prefix = "Env ";
    else if (source_name.startsWithIgnoreCase("lfo"))
      prefix = "Lfo ";
    else if (source_name.startsWithIgnoreCase("vca") || source_name.containsIgnoreCase("master"))
      prefix = "Env ";
    else
      return prefix = "Noise ";

    juce::String digits;
    for (auto character : source_name) {
      if (juce::CharacterFunctions::isDigit(character))
        digits += character;
    }

    return prefix + (digits.isNotEmpty() ? digits : "#");
  }

  juce::Colour getModulationSourceColor(const juce::String& source_name) {
    if (source_name.startsWithIgnoreCase("env"))
      return ShaderColors::kEnvelopeTextColor;
    if (source_name.startsWithIgnoreCase("lfo"))
      return ShaderColors::kLfoTextColor;
    if (source_name.startsWithIgnoreCase("vca") || source_name.containsIgnoreCase("master"))
      return ShaderColors::kMasterEnvelopeTextColor;
    return ShaderColors::kNoise;
  }
}

// custom UI class inheriting from OpenGlToggleButton. When a modulation source has too many connections, instead of displaying
// each one individually next to the button, this appears instead and acts as a collapsed popup
class ExpandModulationButton : public OpenGlToggleButton {
  public:
    ExpandModulationButton() : OpenGlToggleButton("expand modulation"),
                               num_sliders_(0), amount_quad_(Shaders::kRingFragment) {
        setLightenButton();
        setTriggeredOnMouseDown(true);
        setMouseClickGrabsKeyboardFocus(false);
        amount_quad_.setTargetComponent(this);
        amount_quad_.setThickness(2.0f);
    }

    int getNumColumns(int num_sliders) {
        float height_width_ratio = getHeight() * 1.0f / getWidth();
        int columns = 1;
        while (columns * (int)(height_width_ratio * columns) < num_sliders) columns++;
        return columns;
    }

    void setSliders(std::vector<ModulationAmountKnob*> sliders) {
        sliders_ = sliders;
        for (int i = 0; i < sliders.size(); ++i)
            colors_[i] = sliders_[i]->findColour(Skin::kRotaryArc, true);
        num_sliders_ = static_cast<int>(sliders_.size());
    }

    std::vector<ModulationAmountKnob*> getSliders() {
        return sliders_;
    }

    void renderSliderQuads(OpenGlWrapper& open_gl, bool animate) {
        int num_sliders = num_sliders_;
        float width = getWidth();
        float height = getHeight();
        int columns = getNumColumns(num_sliders);
        int rows = (num_sliders + columns - 1) / columns;

        float cell_width = width / columns;
        int y_offset = (height - (rows * cell_width)) / 2;
        float gl_width = 2.0f * cell_width / width;
        float gl_height = 2.0f * cell_width / height;

        int row = 0;
        int column = 0;
        for (int i = 0; i < num_sliders; ++i) {
            float x = column * cell_width;
            float y = height - y_offset - (row + 1) * cell_width;
            amount_quad_.setColor(colors_[i]);
            amount_quad_.setAltColor(colors_[i].withMultipliedAlpha(0.5f));
            amount_quad_.setQuad(0, 2.0f * x / width - 1.0f, 1.0f - 2.0f * y / height - gl_height, gl_width, gl_height);
            amount_quad_.render(open_gl, animate);
            column++;
            if (column >= columns) {
                row++;
                column = 0;
            }
        }
    }

private:
    std::vector<ModulationAmountKnob*> sliders_;
    int num_sliders_;
    juce::Colour colors_[electrosynth::kMaxModulationConnections];
    OpenGlQuad amount_quad_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExpandModulationButton)
};

// represents (wraps) a synthslider, tracks whether it is active/already modulated, computes the visual bounds for the
// drag-over highlight, stores the OpenGl quad index used when drawing destination overlays, handles the three small boxes
// under knobs (extra modulation target boxes)
class ModulationDestination : public juce::Component {
  public:
    ModulationDestination(SynthSlider* source) : destination_slider_(source), margin_(0), index_(0),
                                                 size_multiple_(0.3f),
                                                 active_(false), rectangle_(false), rotary_(true) {
      setComponentID(source->getComponentID());
    }

    ~ModulationDestination() override = default;

    SynthSlider* getDestinationSlider() const {
        return destination_slider_;
    }

    void setActive(bool active) {
        active_ = active;
    }

    void setSizeMultiple(float multiple) {
      size_multiple_ = multiple;
      repaint();
    }

    juce::Rectangle<float> getFillBounds() {

        static constexpr float kBufferPercent = 0.4f;
        float width = getWidth();
        float height = getHeight();

        if (rotary_) {
            float offset = destination_slider_->findValue(Skin::kKnobOffset);
            float rotary_width = size_multiple_ * destination_slider_->findValue(Skin::kKnobModMeterArcSize);
            float x = (width - rotary_width) / 2.0f;
            float y = (height - rotary_width) / 2.0f + offset;
            return juce::Rectangle<float>(x, y, rotary_width, rotary_width);
        }

        if (rectangle_)
            return getLocalBounds().toFloat();

        if (destination_slider_->getSliderStyle() == juce::Slider::LinearBar) {
            float y = height * 0.5f * (1.0f - SynthSlider::kLinearWidthPercent);
            float glow_height = height * SynthSlider::kLinearWidthPercent;
            y -= 2.0f * glow_height * kBufferPercent;
            glow_height += 4.0f * kBufferPercent * glow_height;
            return juce::Rectangle<float>(margin_, y, width - 2 * margin_, glow_height);
      }

      float x = width * 0.5f * (1.0f - SynthSlider::kLinearWidthPercent);
      float glow_width = width * SynthSlider::kLinearWidthPercent;
      x -= 2.0f * glow_width * kBufferPercent;
      glow_width += 4.0f * kBufferPercent * glow_width;
      return juce::Rectangle<float>(x, margin_, glow_width, height - 2 * margin_);

    }

    void setRectangle(bool rectangle) { rectangle_ = rectangle; }
    void setRotary(bool rotary) { rotary_ = rotary; }
    void setMargin(int margin) { margin_ = margin; }
    void setIndex(int index) { index_ = index; }

    bool hasExtraModulationTarget() {
      for (auto* target : destination_slider_->getExtraModulationTargets()) {
        if (target != nullptr)
          return true;
      }
      return false;
    }
    bool isRotary() { return !rectangle_ && rotary_; }
    bool isActive() { return active_; }
    int getIndex() { return index_; }

  private:
    SynthSlider* destination_slider_;
    int margin_;
    int index_;
    float size_multiple_;
    bool active_;
    bool rectangle_;
    bool rotary_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationDestination)
};

// creates the UI knob that controls how much modulation is applied to another slider
ModulationAmountKnob::ModulationAmountKnob(juce::String name, int index, const ValueTree &v) : SynthSlider(name),
                                                                     color_component_(nullptr), index_(index) {
  setModulationKnob(); // set the knob-type as a modulation knob
  bypass_ = false;
  stereo_ = false;
  bipolar_ = false;
  draw_background_ = false;
  name_ = name;
  editing_ = false;

  setShowPopupOnHover(true);
  setTextEntrySizePercent(2.0f, 1.0f);
  setDoubleClickReturnValue(false, 0.0f);
  setWantsKeyboardFocus(false);
  showing_ = true;
  hovering_ = false;
  current_modulator_ = false;
  setRange(-1.f,1.f,0.f);
}

void ModulationAmountKnob::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isMiddleButtonDown()) {
      toggleBypass();
      return;
    }

    if (e.mods.isPopupMenu()) {
        SynthSlider::mouseExit(e);

        PopupItems options;
        options.addItem(kDisconnect, "Remove");
        options.addItem(kToggleBypass, bypass_ ? "Unbypass" : "Bypass");
        options.addItem(kToggleBipolar, bipolar_ ? "Make Unipolar" : "Make Bipolar");
        options.addItem(kToggleStereo, stereo_ ? "Make Mono" : "Make Stereo");
        options.addItem(-1, "");

//    if (has_parameter_assignment_)
//      options.addItem(kArmMidiLearn, "Learn MIDI Assignment");
//
//    if (has_parameter_assignment_ && synth_interface_->getSynth()->isMidiMapped(getComponentID().toStdString()))
//      options.addItem(kClearMidiLearn, "Clear MIDI Assignment");

        options.addItem(kManualEntry, "Enter juce::Value");

        hovering_ = false;
        redoImage();

        auto callback = [=](int selection) {
            handleModulationMenuCallback(selection);
        };
        auto cancel = [=]() {
            for (SliderListener* listener : slider_listeners_)
                listener->menuFinished(this);
        };

        parent_->showPopupSelector(this, e.getPosition(), options, callback, cancel);

        for (SliderListener* listener : slider_listeners_)
            listener->mouseDown(this);
    }

    else {
        SynthSlider::mouseDown(e);
        juce::MouseInputSource source = e.source;

        if (source.isMouse() && source.canDoUnboundedMovement()) {
            editing_ = true;
            source.hideCursor();
            source.enableUnboundedMouseMovement(true);
            mouse_down_position_ = e.getScreenPosition();
            for (SliderListener* listener : slider_listeners_)
                listener->beginModulationEdit(this);
        }
    }
}

void ModulationAmountKnob::mouseUp(const juce::MouseEvent& e) {
    if (!e.mods.isPopupMenu()) {
        SynthSlider::mouseUp(e);
        juce::MouseInputSource source = e.source;

        if (source.isMouse() && source.canDoUnboundedMovement()) {
            source.showMouseCursor(juce::MouseCursor::NormalCursor);
            source.enableUnboundedMouseMovement(false);

            if (getScreenBounds().contains(e.getScreenPosition()))
                editing_ = false;
            source.setScreenPosition(mouse_down_position_.toFloat());
        }
    }

    for (SliderListener* listener : slider_listeners_)
        listener->endModulationEdit(this);

    editing_ = false;
}

void ModulationAmountKnob::mouseExit(const juce::MouseEvent& e) {
    if (!editing_) {
        for (SliderListener* listener : slider_listeners_)
            listener->endModulationEdit(this);
    }

    editing_ = false;
    SynthSlider::mouseExit(e);
}

void ModulationAmountKnob::toggleBypass() {
    bypass_ = !bypass_;
    for (Listener* listener : listeners_)
        listener->setModulationBypass(this, bypass_);

}

void ModulationAmountKnob::handleModulationMenuCallback(int result) {
    if (result == kDisconnect) {
        for (Listener* listener : listeners_)
            listener->disconnectModulation(this);
    }
    else if (result == kToggleBypass)
        toggleBypass();
    else if (result == kToggleBipolar) {
        bipolar_ = !bipolar_;
        for (Listener* listener : listeners_)
            listener->setModulationBipolar(this, bipolar_);
    }
    else if (result == kToggleStereo) {
        stereo_ = !stereo_;
        for (Listener* listener : listeners_)
            listener->setModulationStereo(this, stereo_);
    }
    else
        handlePopupResult(result);

    if (result != kManualEntry) {
        for (SliderListener* listener : slider_listeners_)
            listener->menuFinished(this);
    }
}

void ModulationAmountKnob::makeVisible(bool visible) {
    if (visible == showing_)
        return;
    showing_ = visible;
    setVisible(visible);
    setAlpha((showing_ || hovering_) ? 1.0f : 0.0f);
}

void ModulationAmountKnob::hideImmediately() {
    setAlpha(0.0f, true);
    showing_ = false;
    hovering_ = false;
    setVisible(false);
}

void ModulationAmountKnob::setCurrentModulator(bool current) {
    if (current_modulator_ == current)
        return;

    setColour(Skin::kRotaryArc, findColour(Skin::kModulationMeterControl, true));
    current_modulator_ = current;
}

void ModulationAmountKnob::setSource(const std::string& name) {
    source_name_ = name;
    const auto color = getSourceColor();
    setColour(Skin::kRotaryArc, color);
    setColour(Skin::kRotaryArcUnselected, color.withMultipliedAlpha(0.25f));
    setColour(Skin::kRotaryHand, color);
    setColour(Skin::kModulationMeterControl, color);
    setPopupPrefix(getSourceLabel() + ": ");
    redoImage();
}

juce::String ModulationAmountKnob::getSourceLabel() const {
  return getModulationSourceLabel(source_name_);
}

juce::Colour ModulationAmountKnob::getSourceColor() const {
  return getModulationSourceColor(source_name_);
}


ModulationManager::ModulationManager(ValueTree &tree, SynthBase* base) :
        SynthSection("modulation_manager"), drag_quad_(Shaders::kRingFragment), drag_icon_("modulation_drag_icon"),
        current_modulator_quad_(Shaders::kRoundedRectangleBorderFragment),
        mapping_mode_dim_quad_(Shaders::kColorFragment, "modulation_mapping_mode_dim"),
        editing_rotary_amount_quad_(Shaders::kRotaryModulationFragment),
        editing_linear_amount_quad_(Shaders::kLinearModulationFragment), modifying_(false), dragging_(false),
        changing_hover_modulation_(false),component_update_pending_(false), current_modulator_(nullptr),
        modulation_expansion_box_(std::make_shared<ModulationExpansionBox>()), state_(tree){

    current_modulator_quad_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
    drag_quad_.setTargetComponent(this);

    drag_icon_.setShape(Paths::dragDropArrows());
    drag_icon_.setUseAlpha(true);
    drag_icon_.setActive(false);
    drag_icon_.setInterceptsMouseClicks(false, false);
    addChildComponent(&drag_icon_);

    editing_rotary_amount_quad_.setTargetComponent(this);
    editing_rotary_amount_quad_.setActive(false);
    editing_rotary_amount_quad_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);

    editing_linear_amount_quad_.setTargetComponent(this);
    editing_linear_amount_quad_.setActive(false);
    editing_linear_amount_quad_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);

    addOpenGlComponent(modulation_expansion_box_);
    modulation_expansion_box_->setVisible(false);
    modulation_expansion_box_->setWantsKeyboardFocus(true);
    modulation_expansion_box_->addListener(this);
    modulation_expansion_box_->setAlwaysOnTop(true);

    setSkinOverride(Skin::kModulationDragDrop);

    last_milliseconds_ = juce::Time::currentTimeMillis();
    current_source_ = nullptr;
    current_expanded_modulation_ = nullptr;
    temporarily_set_destination_ = nullptr;
    temporarily_set_synth_slider_ = nullptr;
    temporarily_set_hover_slider_ = nullptr;
    temporarily_set_slot_ = -1;
    temporarily_set_bipolar_ = false;
    setInterceptsMouseClicks(false, true);

    modulation_destinations_ = std::make_unique<juce::Component>();
    modulation_destinations_->setInterceptsMouseClicks(false, true);
    addChildComponent(modulation_destinations_.get());

    mapping_mode_dim_quad_.setTargetComponent(this);
    mapping_mode_dim_quad_.setColor(juce::Colours::black);
    mapping_mode_dim_quad_.setAlpha(0.0f);
    mapping_mode_dim_quad_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);


    electrosynth::ModulationConnectionBank & bank = base->getModulationBank();
    for (int i = 0; i < electrosynth::kMaxModulationConnections; ++i) {
        std::string name = "modulation_" + std::to_string(i + 1) + "_amount";

        // modulation key under slider
        modulation_icon_[i] = std::make_unique<ModulationAmountKnob>(name, i, bank.atIndex(i)->state);
        modulation_icon_[i]->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        addSlider(modulation_icon_[i].get(),true,true);
        modulation_icon_[i]->setAlpha(0.0f, true);
        modulation_icon_[i]->addSliderListener(this);
        modulation_icon_[i]->addModulationAmountListener(this);
        modulation_icon_[i]->setDrawWhenNotVisible(true);
  }

}

void ModulationManager::createModulationMeter(SynthSlider* slider, OpenGlMultiQuad* quads, int index) {
  std::string name = slider->getComponentID().toStdString();

  std::unique_ptr<ModulationMeter> meter = std::make_unique<ModulationMeter>(slider, quads, index);
  addChildComponent(meter.get());
  meter->setName(name);
  meter->setBounds(getLocalArea(slider, slider->getLocalBounds()));
  meter_lookup_[name] = std::move(meter);
}

void ModulationManager::createModulationSlider(std::string name, SynthSlider* slider) {

    std::unique_ptr<ModulationDestination> destination = std::make_unique<ModulationDestination>(slider);
    modulation_destinations_->addAndMakeVisible(destination.get());

    const bool has_slots = std::any_of(
     slider->getExtraModulationTargets().begin(),
     slider->getExtraModulationTargets().end(),
     [] (const auto* target) { return target != nullptr; });

    const bool rotary = slider->isRotary()
                        && !slider->isTextOrCurve(); //&& !has_slots;

    destination->setRectangle(slider->isTextOrCurve());
    destination->setRotary(rotary);
    destination->setSizeMultiple(slider->getKnobSizeScale());

  destination_lookup_[name] = destination.get();
  all_destinations_.push_back(std::move(destination));
}

ModulationManager::~ModulationManager() { }

void ModulationManager::resized() {
  float meter_thickness = findValue(Skin::kKnobModMeterArcThickness);

  juce::Colour meter_center_color = findColour(Skin::kModulationMeter, true);
  juce::Colour meter_left_color = findColour(Skin::kModulationMeterLeft, true);
  juce::Colour meter_right_color = findColour(Skin::kModulationMeterRight, true);

  editing_rotary_amount_quad_.setColor(meter_center_color);
  editing_rotary_amount_quad_.setAltColor(meter_center_color);
  editing_rotary_amount_quad_.setModColor(meter_center_color);
  editing_linear_amount_quad_.setColor(meter_center_color);
  editing_linear_amount_quad_.setAltColor(meter_center_color);
  editing_linear_amount_quad_.setModColor(meter_center_color);


  for (auto& rotary_meter_group : rotary_meters_) {
    rotary_meter_group.second->setThickness(meter_thickness);
    rotary_meter_group.second->setModColor(meter_center_color);
    rotary_meter_group.second->setColor(meter_left_color);
    rotary_meter_group.second->setAltColor(meter_right_color);
  }

  for (auto& linear_meter_group : linear_meters_) {
    linear_meter_group.second->setModColor(meter_center_color);
    linear_meter_group.second->setColor(meter_left_color);
    linear_meter_group.second->setAltColor(meter_right_color);
  }



  modulation_destinations_->setBounds(getLocalBounds());
 // modulation_source_meters_->setBounds(getLocalBounds());

  updateModulationMeterLocations();

  juce::Colour meter_control = findColour(Skin::kModulationMeterControl, true);
  current_modulator_quad_.setColor(meter_control);
  drag_quad_.setColor(meter_control);
  drag_quad_.setThumbColor(meter_control);
  drag_quad_.setAltColor(findColour(Skin::kWidgetBackground, true));


    modulation_expansion_box_->setColor(findColour(Skin::kBody, true));
    // set destination map colors
  juce::Colour lighten_screen = findColour(Skin::kLightenScreen, true);
  float rounding = parent_->findValue(Skin::kLabelBackgroundRounding);

  for (auto& rotary_destination_group : rotary_destinations_)
    rotary_destination_group.second->setColor(lighten_screen);

  for (auto& linear_destination_group : linear_destinations_) {
    linear_destination_group.second->setColor(lighten_screen);
    linear_destination_group.second->setRounding(rounding);
  }

  SynthSection::resized();
  clearModulationSource();
}

void ModulationManager::parentHierarchyChanged() {
  SynthSection::parentHierarchyChanged();
//  if (!modulation_source_readouts_.empty())
//    return;

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

//  for (auto& mod_button : modulation_buttons_) {
//    modulation_source_readouts_[mod_button.first] = parent->getSynth()->getStatusOutput(mod_button.first);
//    smooth_mod_values_[mod_button.first] = 0.0f;
//    active_mod_values_[mod_button.first] = false;
//  }
//
//  num_voices_readout_ = parent->getSynth()->getStatusOutput("num_voices");
}

void ModulationManager::updateModulationMeterLocations() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();

  for (auto& meter : meter_lookup_) {
    SynthSlider* model = slider_model_lookup_[meter.first];
    if (model)
      meter.second->setBounds(getLocalArea(model, model->getModulationMeterBounds()));

    if (parent) {
      int num_modulations = parent->getSynth()->getNumModulations(meter.first);
      meter.second->setModulated(num_modulations);
      meter.second->setVisible(num_modulations);
    }
  }
}

void ModulationManager::modulationAmountChanged(SynthSlider* slider) {
  std::string slider_name = slider->getComponentID().toStdString();
  std::string source_name = current_modulator_->getComponentID().toStdString();
  setModulationValues(source_name, slider_name,
                      slider->getModulationAmount(), slider->isModulationBipolar(),
                      slider->isModulationStereo(), slider->isModulationBypassed());
  modulation_buttons_[source_name]->repaint();
}

void ModulationManager::modulationRemoved(SynthSlider* slider) {
  std::string slider_name = slider->getComponentID().toStdString();
  std::string source_name = current_modulator_->getComponentID().toStdString();

  removeModulation(source_name, slider_name);
  modulation_buttons_[source_name]->repaint();
}

void ModulationManager::modulationDisconnected(electrosynth::ModulationConnection* connection, bool last) {
  if (current_modulator_ == nullptr)
    return;

  if (meter_lookup_.count(connection->destination_name)) {
    meter_lookup_[connection->destination_name]->setModulated(!last);
    meter_lookup_[connection->destination_name]->setVisible(!last);
  }
}

void ModulationManager::modulationSelected(ModulationButton* source) {
  for (auto& button : modulation_buttons_)
    button.second->setActiveModulation(button.second == source);

  current_modulator_ = source;
  for (auto& hover_slider : modulation_icon_)
    hover_slider->makeVisible(false);
  makeCurrentModulatorAmountsVisible();
  setModulationAmounts();
}

void ModulationManager::modulationClicked(ModulationButton* source) {
  hideUnusedHoverModulations();
}

void ModulationManager::modulationCleared() {
  makeCurrentModulatorAmountsVisible();
}

bool ModulationManager::hasFreeConnection() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  electrosynth::ModulationConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < electrosynth::kMaxModulationConnections; ++i) {
    electrosynth::ModulationConnection* connection = bank.atIndex(i);
    if (connection->source_name.empty() && connection->destination_name.empty())
      return true;
  }

  return false;
}

void ModulationManager::scheduleComponentUpdate()
{
  if (component_update_pending_)
    return;                          // coalesce repeated add/remove triggers
  component_update_pending_ = true;
  juce::Component::SafePointer<ModulationManager> safe_this(this);
  juce::MessageManager::callAsync([safe_this]() {
    if (safe_this == nullptr)
      return;                        // manager destroyed before this turn ran
    safe_this->component_update_pending_ = false;  // reset before rebuild so
                                     // componentAdded()'s not-ready retry logic still works
    safe_this->componentAdded();
  });
}

void ModulationManager::componentAdded()
{
  FullInterface* full = findParentComponentOfClass<FullInterface>();
  if (full == nullptr || !full->open_gl_.context.isAttached() || full->open_gl_.shaders == nullptr) {
    if (!component_update_pending_) {
      component_update_pending_ = true;
      juce::Component::SafePointer<ModulationManager> safe_this(this);
      juce::Timer::callAfterDelay(50, [safe_this]() {
        if (safe_this == nullptr)
          return;
        safe_this->component_update_pending_ = false;
        safe_this->componentAdded();
            });
        }
        return;
    }

  component_update_pending_ = false;

  // Async ownership handoff (replaces a blocking executeOnGLThread(...,true) that
  // deadlocked the message thread -> watchdog SIGKILL when reached via removeModule
  // -> listener->removed()). Move the old GL-backed modulation multiquads out of the
  // active maps into a heap keep-alive, destroy their GL resources on the GL thread
  // (non-blocking), then drop the keep-alive back on the message thread so the C++
  // destructors run there (matching the prior behavior, where destruction happened at
  // the message-thread .clear() below).
  struct OldModResources {
      std::map<juce::Viewport*, std::shared_ptr<OpenGlMultiQuad>> rotary_destinations;
      std::map<juce::Viewport*, std::unique_ptr<OpenGlMultiQuad>> linear_destinations;
      std::map<juce::Viewport*, std::shared_ptr<OpenGlMultiQuad>> rotary_meters;
      std::map<juce::Viewport*, std::unique_ptr<OpenGlMultiQuad>> linear_meters;
  };
  auto old_resources = std::make_shared<OldModResources>();
  {
      // Move under the GL lock so we don't race the renderer reading these maps.
      ScopedLock lock (open_gl_critical_section_);
      old_resources->rotary_destinations = std::move (rotary_destinations_);
      old_resources->linear_destinations = std::move (linear_destinations_);
      old_resources->rotary_meters       = std::move (rotary_meters_);
      old_resources->linear_meters       = std::move (linear_meters_);
      rotary_destinations_.clear();
      linear_destinations_.clear();
      rotary_meters_.clear();
      linear_meters_.clear();
  }

  full->open_gl_.context.executeOnGLThread ([old_resources] (juce::OpenGLContext& openGLContext) {
      for (auto& multiquad : old_resources->rotary_destinations)
          multiquad.second->destroy (openGLContext);
      for (auto& multiquad : old_resources->rotary_meters)
          multiquad.second->destroy (openGLContext);
      for (auto& multiquad : old_resources->linear_meters)
          multiquad.second->destroy (openGLContext);
      for (auto& multiquad : old_resources->linear_destinations)
          multiquad.second->destroy (openGLContext);
      // Drop the final reference on the message thread (C++ destruction off the GL thread).
      juce::MessageManager::callAsync ([old_resources]() mutable {
          old_resources.reset();
      });
  },
      false); // non-blocking: do NOT park the message thread

    auto sliders = full->getAllSliders();
    auto mod_buttons = full->getAllModulationButtons();

    {
        ScopedLock lock (open_gl_critical_section_);

        auto contains_current_modulator = [&mod_buttons] (ModulationButton* button) {
            if (button == nullptr)
                return false;
            for (const auto& modulation_button : mod_buttons)
                if (modulation_button.second == button)
                    return true;
            return false;
        };

        if (!contains_current_modulator(current_modulator_))
            current_modulator_ = nullptr;
        if (!contains_current_modulator(current_source_))
            current_source_ = nullptr;
        current_expanded_modulation_ = nullptr;
        modulation_expansion_box_->setVisible(false);


        dragging_ = false;
        current_source_ = nullptr;
        current_modulator_ = nullptr;
        temporarily_set_destination_ = nullptr;
        temporarily_set_synth_slider_ = nullptr;
        temporarily_set_hover_slider_ = nullptr;
        temporarily_set_slot_ = -1;
        modulation_destinations_->setVisible(false);


        rotary_destinations_.clear();
        rotary_meters_.clear();
        linear_destinations_.clear();
        linear_meters_.clear();
        destination_lookup_.clear();
        all_destinations_.clear();
        modulation_buttons_.clear();
        modulation_callout_buttons_.clear();
        meter_lookup_.clear();
        num_linear_meters.clear();
        num_rotary_meters.clear();
        modulation_buttons_ = mod_buttons;
        for (auto& modulation_button : modulation_buttons_) {
            modulation_button.second->addListener(this);
            modulation_callout_buttons_[modulation_button.first] = std::make_unique<ExpandModulationButton>();
            addChildComponent(modulation_callout_buttons_[modulation_button.first].get());
            // addOpenGlComponent(modulation_callout_buttons_[modulation_button.first]->getGlComponent());
            modulation_callout_buttons_[modulation_button.first]->addListener(this);
        }
        slider_model_lookup_.clear();
        slider_model_lookup_ = sliders;
        for (auto& slider : slider_model_lookup_) {
                //        if (mono_modulations[slider.first]) {
            std::string name = slider.first;
            const bool has_slots = std::any_of(
                    slider.second->getExtraModulationTargets().begin(),
                    slider.second->getExtraModulationTargets().end(),
                    [] (const auto* target) { return target != nullptr; });

            const bool rotary = slider.second->isRotary() && !slider.second->isTextOrCurve(); // && !has_slots;
            const bool linear = !rotary;

                juce::Viewport* viewport = slider.second->findParentComponentOfClass<juce::Viewport>();
                if (rotary)
                    num_rotary_meters[viewport] = num_rotary_meters[viewport] + 1;
                else if (linear)
                    num_linear_meters[viewport] += has_slots ? SynthSlider::kNumModulationSlots : 1;
            }


        for (auto& rotary_meters : num_rotary_meters) {
            //DBG ("num rotary" + String (rotary_meters.second));
            rotary_destinations_[rotary_meters.first] = std::make_unique<OpenGlMultiQuad> (rotary_meters.second,
                Shaders::kRingFragment); //kCircleFragment
            rotary_destinations_[rotary_meters.first]->setThickness (55.0f);
            rotary_destinations_[rotary_meters.first]->setTargetComponent (this);
            rotary_destinations_[rotary_meters.first]->setScissorComponent (rotary_meters.first);
            rotary_destinations_[rotary_meters.first]->setAlpha (0.0f, true); //DEBUG FIX

            rotary_meters_[rotary_meters.first] = std::make_unique<OpenGlMultiQuad> (rotary_meters.second,
                Shaders::kRotaryModulationFragment);
            rotary_meters_[rotary_meters.first]->setTargetComponent (this);
            rotary_meters_[rotary_meters.first]->setScissorComponent (rotary_meters.first);
            rotary_meters_[rotary_meters.first]->setAlpha (1.0f, true);
            rotary_meters_[rotary_meters.first]->setVisible(true);
        }
        for (auto& linear_meters : num_linear_meters)
        {
            linear_destinations_[linear_meters.first] = std::make_unique<OpenGlMultiQuad> (
                linear_meters.second,
                Shaders::kRoundedRectangleFragment);

            linear_destinations_[linear_meters.first]->setTargetComponent (this);
            linear_destinations_[linear_meters.first]->setScissorComponent (linear_meters.first);
            linear_destinations_[linear_meters.first]->setAlpha (0.0f, true);

            linear_meters_[linear_meters.first] = std::make_unique<OpenGlMultiQuad> (linear_meters.second,
                Shaders::kLinearModulationFragment);
            linear_meters_[linear_meters.first]->setTargetComponent (this);
            linear_meters_[linear_meters.first]->setScissorComponent (linear_meters.first);
        }
        for (auto& slider : slider_model_lookup_) {
            const std::string name = slider.first;

            const bool rotary = slider.second->isRotary() && !slider.second->isTextOrCurve();
            const bool linear = !rotary;
            Viewport* viewport = slider.second->findParentComponentOfClass<Viewport>();

            if (rotary) {
                int index = num_rotary_meters[viewport] - 1;
                num_rotary_meters[viewport] = index;
                createModulationMeter(slider.second, rotary_meters_[viewport].get(), index);
            }
            else if (linear) {
                int index = num_linear_meters[viewport] - 1;
                num_linear_meters[viewport] = index;
                createModulationMeter (slider.second, linear_meters_[viewport].get(), index);
            }

            slider.second->addSliderListener (this);
            createModulationSlider (name, slider.second);
        }
    }

    updateModulationSlotVisuals();
    full->open_gl_.context.executeOnGLThread ([this, full] (juce::OpenGLContext& openGLContext) {
        for (auto& multiquad : rotary_destinations_)
        {
            multiquad.second->init (full->open_gl_);
        }
        for (auto& multiquad : rotary_meters_)
        {
            multiquad.second->init (full->open_gl_);
        }
        for (auto& multiquad : linear_meters_)
        {
            multiquad.second->init (full->open_gl_);
        }
        for (auto& multiquad : linear_destinations_)
        {
            multiquad.second->init (full->open_gl_);
        }
    },
        true);

    resized();
}

bool ModulationManager::isMappingMode() const {
    return dragging_ && current_modulator_ != nullptr;
}

void ModulationManager::drawMappingMode(OpenGlWrapper& open_gl) {
    if (!isMappingMode()) {
        mapping_mode_dim_quad_.setAlpha(0.0f);
        return;
    }
    mapping_mode_dim_quad_.setTargetComponent(this);
    mapping_mode_dim_quad_.setColor(juce::Colours::black);
    mapping_mode_dim_quad_.setAlpha(0.45f);
    mapping_mode_dim_quad_.render(open_gl, true);
}

void ModulationManager::startDestinationMap(ModulationButton* source, const juce::MouseEvent& e) {
  if (!hasFreeConnection()) return;

  mouse_drag_position_ = getLocalPoint(source, e.getPosition());
  current_source_ = source;
  dragging_ = true;
  positionDragIcon();
  juce::Rectangle<int> global_bounds = getLocalArea(current_source_, current_source_->getLocalBounds());
  juce::Point<int> global_start = global_bounds.getCentre();
  mouse_drag_start_ = global_start;
  modulation_destinations_->setVisible(true);
  int widget_margin = findValue(Skin::kWidgetMargin);

  std::map<juce::Viewport*, int> rotary_indices;
  std::map<juce::Viewport*, int> linear_indices;
  for (auto& rotary_destination_group : rotary_destinations_)
    rotary_indices[rotary_destination_group.first] = 0;

  for (auto& linear_destination_group : linear_destinations_)
    linear_indices[linear_destination_group.first] = 0;

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  std::string source_name = source->getComponentID().toStdString();
  std::set<std::string> active_destinations;
  std::vector<electrosynth::ModulationConnection*> connections = parent->getSynth()->getSourceConnections(source_name);
  for (electrosynth::ModulationConnection* connection : connections)
    active_destinations.insert(connection->destination_name);

    for (auto& destination : destination_lookup_) {

        auto slider_iter = slider_model_lookup_.find(destination.first);
        if (slider_iter == slider_model_lookup_.end() || slider_iter->second == nullptr) continue;

        SynthSlider* model = slider_iter->second;
        if (current_source_ == nullptr) continue;

        bool should_show = model->isShowing() && model->getSectionParent()->isActive() && current_source_->getComponentID() != juce::String(destination.first);

        juce::Viewport* viewport = model->findParentComponentOfClass<juce::Viewport>();
        destination.second->setVisible(should_show);
        destination.second->setActive(active_destinations.count(destination.first));
        destination.second->setMargin(widget_margin);

        juce::Point<int> position = getLocalPoint(model, juce::Point<int>(0, 0));
        juce::Rectangle<int> slider_bounds = (model->getLocalBounds() + position).reduced(5.f);
        destination.second->setBounds(slider_bounds);

        if (should_show) {
            if (destination.second->isRotary()) {
                destination.second->setIndex(rotary_indices[viewport]);
                rotary_indices[viewport] = rotary_indices[viewport] + 1;
            }

            else {
                destination.second->setIndex(linear_indices[viewport]);
                linear_indices[viewport] += destination.second->hasExtraModulationTarget()
                                      ? SynthSlider::kNumModulationSlots : 1;
            }
            setDestinationQuadBounds(destination.second);
        }
    }
 //DEBUG FIX
    for (auto& index_count : rotary_indices) {
        rotary_destinations_[index_count.first]->setNumQuads(index_count.second);
        rotary_destinations_[index_count.first]->setAlpha(index_count.second > 0 ? 1.0f : 0.0f);
    }

    for (auto& index_count : linear_indices) {
        linear_destinations_[index_count.first]->setNumQuads(index_count.second);
        linear_destinations_[index_count.first]->setAlpha(index_count.second > 0 ? 1.0f : 0.0f);
    }
}

void ModulationManager::setDestinationQuadBounds(ModulationDestination* destination) {

  juce::Viewport* viewport =
      destination->getDestinationSlider()->findParentComponentOfClass<juce::Viewport>();

  juce::Point<float> top_left = destination->getBounds().getTopLeft().toFloat();
  juce::Rectangle<float> draw_bounds = destination->getLocalBounds().toFloat() + top_left;
  draw_bounds = destination->getFillBounds() + top_left;

  float global_width = getWidth();
  float global_height = getHeight();
  float x = 2.0f * draw_bounds.getX() / global_width - 1.0f;
  float y = 1.0f - 2.0f * draw_bounds.getBottom() / global_height;
  float width = 2.0f * draw_bounds.getWidth() / global_width;
  float height = 2.0f * draw_bounds.getHeight() / global_height;

  float offset = destination->isActive() ? -2.0f : 0.0f;

  if (destination->isRotary()) {
      rotary_destinations_[viewport]->setQuad(destination->getIndex(), x + offset, y, width, height);
  }
  else
    linear_destinations_[viewport]->setQuad(destination->getIndex(), x + offset, y, width, height);
}

bool ModulationManager::isPointInsideDestinationDropArea(SynthSlider* slider, juce::Point<int> manager_position) const {
    if (slider == nullptr) return false;

    const auto slider_top_left = getLocalPoint (slider, juce::Point<int>());
    const auto slider_bounds = (slider->getLocalBounds() + slider_top_left).toFloat();
    const float radius = 0.65f * std::min(slider_bounds.getWidth(), slider_bounds.getHeight());

    // if the mouse is outside the radius, return
    const auto center = slider_bounds.getCentre();
    if (manager_position.toFloat().getDistanceFrom(center) < radius) return true;

    return false;
}

int ModulationManager::findSlotForNewConnection(SynthSlider* slider) const {
    if (slider == nullptr) return -1;

    const std::string destination = slider->getComponentID().toStdString();
    const auto& targets = slider->getExtraModulationTargets();

    if (temporarily_set_synth_slider_ == slider && temporarily_set_slot_ >= 0) return temporarily_set_slot_;

    for (int slot = 0; slot < SynthSlider::kNumModulationSlots; slot++)
    {
        auto* target = targets[slot];
        if (target == nullptr || !target->isShowing()) continue;
        if (!isModulationSlotOccupied (destination, slot)) return slot;
    }

    return -1;
}

bool ModulationManager::isModulationSlotOccupied(const std::string& destination, int destination_slot) const {
  if (destination_slot < 0)
    return false;

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return false;

  for (auto* connection : parent->getSynth()->getDestinationConnections(destination)) {
    if (connection->destination_slot == destination_slot)
      return true;
  }

  return false;
}

void ModulationManager::updateModulationSlotVisuals() {
		SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
		if (parent == nullptr) return;

    std::vector<electrosynth::ModulationSlotComponent*> active_slots;

	    auto get_display_label = [this](const std::string& source_name) {
	        if (auto button = modulation_buttons_.find(source_name);
	            button != modulation_buttons_.end() && button->second != nullptr)
            return button->second->getDisplayLabel();

        return juce::String();
    };

	auto& bank = parent->getSynth()->getModulationBank();
	for (int index = 0; index < electrosynth::kMaxModulationConnections; ++index) {
		auto* connection = bank.atIndex(index);
        if (connection == nullptr || connection->destination_name.empty()
                || !juce::isPositiveAndBelow(connection->destination_slot, SynthSlider::kNumModulationSlots))
            continue;

        auto slider = slider_model_lookup_.find(connection->destination_name);
        if (slider == slider_model_lookup_.end() || slider->second == nullptr) continue;

	    auto* target = slider->second->getExtraModulationTarget(connection->destination_slot);
	    if (auto* slot = dynamic_cast<electrosynth::ModulationSlotComponent*>(target)) {
            active_slots.push_back(slot);
	        slot->setSourceName(connection->source_name);
	        slot->setSourceDisplayLabel(get_display_label(connection->source_name));
	        slot->setModulationAmount(connection->getCurrentBaseValue());
	        slot->setBypass(connection->isBypass());

            bool has_aux = false;
            if (auto aux = aux_connections_to_from_.find(connection->index_in_all_mods);
                aux != aux_connections_to_from_.end()) {
                if (auto* aux_connection = bank.atIndex(aux->second);
                    aux_connection != nullptr && !aux_connection->source_name.empty()) {
	                    slot->setAuxSource(aux_connection->source_name, get_display_label(aux_connection->source_name));
                    has_aux = true;
                }
            }
            if (!has_aux)
                slot->setAuxSource({}, {});
	    }
		}

    for (const auto& [name, slider] : slider_model_lookup_) {
        if (slider == nullptr) continue;

        for (auto* target : slider->getExtraModulationTargets()) {
            auto* slot = dynamic_cast<electrosynth::ModulationSlotComponent*>(target);
            if (slot == nullptr)
                continue;

            if (std::find(active_slots.begin(), active_slots.end(), slot) == active_slots.end())
                slot->clearSource();
        }
    }

	  // Parameter views inside sound/effect modules are rendered into cached
  // background images. Rebuild the full background after all slot states have
  // been updated so their source-colored icons are included in those caches.
    if (auto* full = parent->getGui())
        full->redoBackground();
}

// creates an auxiliary modulation connection to an existing modulation connection
void ModulationManager::makeAuxilaryModulationConnection(ModulationAmountKnob* hover_slider) {
    if (hover_slider->isCurrentModulator() || hover_slider->hasAux() || current_modulator_ == nullptr)
        return;

    std::string name = hover_slider->getOriginalName().toStdString();
    std::string source_name = current_modulator_->getComponentID().toStdString();
    electrosynth::ModulationConnection* connection = getConnection(source_name, name);
    if (connection == nullptr) {
        float value = hover_slider->getValue() * 0.5f;
        hover_slider->setValue(0.0f, sendNotificationSync);
        temporarily_set_hover_slider_ = hover_slider;

        if (!connectModulation(source_name, name)) return;

        setModulationValues(source_name, name, value, false, false, false);
        connection = getConnection(source_name, name);
        if (connection == nullptr) return;

        int new_index = connection->index_in_all_mods;
        addAuxConnection(new_index, hover_slider->index());
        setModulationSliderValues(new_index, value);
    }
}

void ModulationManager::modulationDraggedToComponent(juce::Component* component, bool bipolar) {
    if (component == nullptr || current_modulator_ == nullptr)
        return;

    std::string destination_name = component->getComponentID().toStdString();
    DBG("destination_name: " + destination_name);
    auto destination_iter = destination_lookup_.find(destination_name);
    if (destination_iter == destination_lookup_.end() || destination_iter->second == nullptr)
        return;

    ModulationDestination* destination = destination_iter->second;
    SynthSlider* slider = destination->getDestinationSlider();
    if (slider == nullptr)
        return;

    if (!isPointInsideDestinationDropArea(slider, mouse_drag_position_)) {
        if (temporarily_set_destination_ == destination)
            clearTemporaryModulation();
        return;
    }
    auto const source_name = current_modulator_->getComponentID().toStdString();

    if (getConnection(source_name, destination_name) != nullptr)
        return; // already connected, no preview creation

    const int destination_slot = findSlotForNewConnection(slider);
    if (isModulationSlotOccupied(destination_name, destination_slot)) return; // if slot is taken, return

    if (destination_slot < 0) return;
    if (temporarily_set_destination_ == destination && temporarily_set_slot_ != destination_slot)
        clearTemporaryModulation();



    float percent = slider->valueToProportionOfLength(slider->getValue());
    float modulation_amount = 1.0f - percent;
    if (bipolar) modulation_amount = std::min(modulation_amount, percent) * 2.0f;
    modulation_amount = std::max(modulation_amount, kDefaultModulationRatio);

    if (!connectModulation(source_name, destination_name, destination_slot)) return;

    temporarily_set_destination_ = destination;
    temporarily_set_synth_slider_ = slider_model_lookup_[destination_name];
    temporarily_set_slot_ = destination_slot;
    updateModulationSlotVisuals();
    setModulationValues(source_name, destination_name, modulation_amount, bipolar, false, false, destination_slot);
    destination->setActive(true);
    setDestinationQuadBounds(destination);

    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    std::vector<electrosynth::ModulationConnection*> connections = parent->getSynth()->getDestinationConnections(destination_name);

    for (electrosynth::ModulationConnection* connection : connections) {
      if (connection->source_name == source_name
          && connection->destination_name == destination_name
          && connection->destination_slot == destination_slot) {
        int index = connection->index_in_all_mods;
        showModulationAmountOverlay(modulation_icon_[index].get());
      }
    }

    setVisibleMeterBounds();
    makeModulationsVisible(slider, true);
    DBG("modconnecte4d");
}

void ModulationManager::setTemporaryModulationBipolar(juce::Component* component, bool bipolar) {
  if (current_modulator_ == nullptr || component != temporarily_set_destination_ || component == nullptr)
    return;

  std::string source_name = current_modulator_->getComponentID().toStdString();
  std::string name = component->getComponentID().toStdString();
  ModulationDestination* destination = destination_lookup_[name];
  SynthSlider* slider = destination->getDestinationSlider();

  float percent = slider->valueToProportionOfLength(slider->getValue());
  float modulation_amount = 1.0f - percent;
  if (bipolar)
    modulation_amount = std::min(modulation_amount, percent) * 2.0f;
  modulation_amount = std::max(modulation_amount, kDefaultModulationRatio);

  int index = getModulationIndex(source_name, name, temporarily_set_slot_);
  setModulationValues(source_name, name, modulation_amount, bipolar, false, false, temporarily_set_slot_);
  temporarily_set_bipolar_ = bipolar;
  if (juce::isPositiveAndBelow(index, electrosynth::kMaxModulationConnections))
    showModulationAmountOverlay(modulation_icon_[index].get());
}

void ModulationManager::clearTemporaryModulation() {
  if (temporarily_set_destination_ && current_modulator_) {
    auto* destination = temporarily_set_destination_;
    destination->setActive(false);
    std::string source_name = current_modulator_->getComponentID().toStdString();
    removeModulation(source_name, temporarily_set_synth_slider_->getComponentID().toStdString(),
                     temporarily_set_slot_);
    setDestinationQuadBounds(destination);
    temporarily_set_destination_ = nullptr;
    temporarily_set_synth_slider_ = nullptr;
    temporarily_set_slot_ = -1;
    updateModulationSlotVisuals();

    hideModulationAmountOverlay();
  }
}

void ModulationManager::clearTemporaryHoverModulation() {
  if (temporarily_set_hover_slider_ && current_modulator_) {
    std::string name = temporarily_set_hover_slider_->getOriginalName().toStdString();

    std::string source_name = current_modulator_->getComponentID().toStdString();
    removeModulation(source_name, temporarily_set_hover_slider_->getOriginalName().toStdString());
    temporarily_set_hover_slider_ = nullptr;
  }
}

void ModulationManager::modulationDragged(const juce::MouseEvent& e) {
  if (!dragging_) return;

  mouse_drag_position_ = getLocalPoint(current_source_, e.getPosition());
  positionDragIcon();
  juce::Component* component = nullptr;

  // Resolve slot destinations directly from the three visible box components.
  // This avoids relying on Component::getComponentAt() to choose between the
  // destination overlay and the underlying UI hierarchy.
  for (const auto& [name, destination] : destination_lookup_) {
    if (destination == nullptr || !destination->isVisible())
      continue;

    if (isPointInsideDestinationDropArea(destination->getDestinationSlider(), mouse_drag_position_)) {
      component = destination;
      break;
    }
  }

  if (component == nullptr)
    component = getComponentAt(mouse_drag_position_.x, mouse_drag_position_.y);

  ModulationAmountKnob* hover_knob = nullptr;
  for (int i = 0; i < electrosynth::kMaxModulationConnections; ++i) {
    if (modulation_icon_[i].get() == component)
      hover_knob = modulation_icon_[i].get();
  }

  if (hover_knob && hover_knob->isCurrentModulator())
    return;

  bool bipolar = e.mods.isAnyModifierKeyDown();
  if (temporarily_set_destination_ && temporarily_set_destination_ != component)
    clearTemporaryModulation();
  if (temporarily_set_hover_slider_ && temporarily_set_hover_slider_ != component)
    clearTemporaryHoverModulation();

  else if (temporarily_set_synth_slider_ && temporarily_set_bipolar_ != bipolar)
    setTemporaryModulationBipolar(component, bipolar);

  if (hover_knob)
    makeAuxilaryModulationConnection(hover_knob);
  else
    modulationDraggedToComponent(component, bipolar);
}

void ModulationManager::modulationWheelMoved(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
  if (!dragging_ || current_modulator_ == nullptr || temporarily_set_destination_ == nullptr)
    return;

  juce::MouseEvent new_event(e.source, e.position, juce::ModifierKeys(), e.pressure, e.orientation, e.rotation,
                       e.tiltX, e.tiltY, e.eventComponent, e.originalComponent, e.eventTime, e.mouseDownPosition,
                       e.mouseDownTime, e.getNumberOfClicks(), e.mouseWasDraggedSinceMouseDown());
  std::string source_name = current_modulator_->getComponentID().toStdString();
  std::string destination_name = temporarily_set_destination_->getComponentID().toStdString();
  int index = getModulationIndex(source_name, destination_name, temporarily_set_slot_);
  if (index >= 0)
    modulation_icon_[index]->mouseWheelMove(new_event, wheel);
}

void ModulationManager::endModulationMap() {
  temporarily_set_destination_ = nullptr;
  temporarily_set_synth_slider_ = nullptr;
  temporarily_set_hover_slider_ = nullptr;
  temporarily_set_slot_ = -1;
  dragging_ = false;

  setModulationAmounts();
  current_source_ = nullptr;
  for (auto& rotary_destination_group : rotary_destinations_)
    rotary_destination_group.second->setAlpha(0.0f);

  for (auto& linear_destination_group : linear_destinations_)
    linear_destination_group.second->setAlpha(0.0f);

  modulation_destinations_->setVisible(false);
  drag_quad_.setThickness(0.0f, true);
  drag_icon_.setVisible(false);
  drag_icon_.setActive(false);
  hideModulationAmountOverlay();
}

void ModulationManager::modulationLostFocus(ModulationButton* source) {
  source->setActiveModulation(false);
  clearModulationSource();
}

void ModulationManager::clearModulationSource() {
  if (current_modulator_) {
    for (auto& selected_slider : modulation_icon_)
      selected_slider->makeVisible(false);
  }
  current_modulator_ = nullptr;
  setModulationAmounts();
}

void ModulationManager::disconnectModulation(ModulationAmountKnob* modulation_knob) {

  electrosynth::ModulationConnection* connection = getConnectionForModulationSlider(modulation_knob);
  while (connection && !connection->source_name.empty() && !connection->destination_name.empty()) {
    removeModulation(connection->source_name, connection->destination_name, connection->destination_slot);
    connection = getConnectionForModulationSlider(modulation_knob);
  }

  setModulationAmounts();
}

void ModulationManager::setModulationSettings(ModulationAmountKnob* modulation_knob) {
  electrosynth::ModulationConnection* connection = getConnectionForModulationSlider(modulation_knob);
  float value = modulation_knob->getValue();
  bool bipolar = modulation_knob->isBipolar();
  bool stereo = modulation_knob->isStereo();
  bool bypass = modulation_knob->isBypass();

  int index = modulation_knob->index();
  modulation_icon_[index]->setBipolar(bipolar);
  modulation_icon_[index]->setStereo(stereo);
  modulation_icon_[index]->setBypass(bypass);

  setModulationValues(connection->source_name, connection->destination_name, value, bipolar, stereo, bypass,
                      connection->destination_slot);
}

void ModulationManager::setModulationBypass(ModulationAmountKnob* modulation_knob, bool bypass) {
  setModulationSettings(modulation_knob);
}

void ModulationManager::setModulationBipolar(ModulationAmountKnob* modulation_knob, bool bipolar) {
  setModulationSettings(modulation_knob);
}

void ModulationManager::setModulationStereo(ModulationAmountKnob* modulation_knob, bool stereo) {
  setModulationSettings(modulation_knob);
}

void ModulationManager::initOpenGlComponents(OpenGlWrapper& open_gl) {
    drag_quad_.init(open_gl);
    drag_icon_.init(open_gl);
    mapping_mode_dim_quad_.init(open_gl);
    modulation_expansion_box_->init(open_gl);

    if (modulation_source_meters_)
        modulation_source_meters_->init(open_gl);
    for (auto& rotary_destination_group : rotary_destinations_)
        rotary_destination_group.second->init(open_gl);

    for (auto& linear_destination_group : linear_destinations_)
        linear_destination_group.second->init(open_gl);

    for (auto& rotary_meter_group : rotary_meters_)
        rotary_meter_group.second->init(open_gl);

    for (auto& linear_meter_group : linear_meters_)
        linear_meter_group.second->init(open_gl);

    SynthSection::initOpenGlComponents(open_gl);
}

void ModulationManager::drawModulationDestinations(OpenGlWrapper& open_gl) {
    const bool mapping_mode = isMappingMode();
    auto destination_color = findColour(Skin::kLightenScreen, true).brighter (1.0);
    if (mapping_mode)
        destination_color = current_source_ != nullptr ?
                current_source_->getSourceColor() : destination_color;


    for (auto& rotary_destination_group : rotary_destinations_) {
        rotary_destination_group.second->setColor(destination_color);
        rotary_destination_group.second->setAlpha(mapping_mode ? 0.4f : 0.0f);
        rotary_destination_group.second->render(open_gl, true);
    }

    for (auto& linear_destination_group : linear_destinations_) {
        linear_destination_group.second->setColor(destination_color);
        linear_destination_group.second->setAlpha(mapping_mode ? 0.4f : 0.0f);
        linear_destination_group.second->render(open_gl, true);
}
}

void ModulationManager::drawCurrentModulator(OpenGlWrapper& open_gl) {
  juce::Component* component = current_modulator_;
  if (component) {
    current_modulator_quad_.setTargetComponent(component);
    if (auto* mod_button = dynamic_cast<ModulationButton*>(component))
      current_modulator_quad_.setColor(mod_button->getSourceColor());
    current_modulator_quad_.setAlpha(1.0f);
  }
  else
    current_modulator_quad_.setAlpha(0.0f);

  current_modulator_quad_.setThickness(dragging_ ? 3.0f : 1.0f);
  current_modulator_quad_.render(open_gl, true);
}

void ModulationManager::positionDragIcon() {
  static constexpr float kRadiusWidthRatio = 0.03f;
  if (current_source_ == nullptr || getWidth() <= 0 || getHeight() <= 0) return;

  const int icon_size = static_cast<int>(std::round(kRadiusWidthRatio * getWidth()));
  const Rectangle<int> bounds(mouse_drag_position_.x - icon_size / 2, mouse_drag_position_.y - icon_size / 2,
                                    icon_size, icon_size);
  if (drag_icon_.getBounds() != bounds) drag_icon_.setBounds(bounds);

    drag_icon_.setActive(true);
    drag_icon_.setVisible(true);
    drag_icon_.setColor(current_source_->getSourceColor());
    drag_icon_.redrawImage(true);
}

void ModulationManager::drawDraggingModulation(OpenGlWrapper& open_gl) {
  if (current_source_ == nullptr || temporarily_set_destination_ || temporarily_set_hover_slider_)
    return;

  drag_icon_.setActive(true);
  drag_icon_.render(open_gl, true);
}

void ModulationManager::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
    if (!animate)
        return;

    ScopedLock lock(open_gl_critical_section_);

    drawMappingMode(open_gl);
    SynthSection::renderOpenGlComponents(open_gl, animate); // render existing child/open-gl components
    OpenGlComponent::setViewPort(this, open_gl);


    for (auto& callout_button : modulation_callout_buttons_) {
        if (callout_button.second->isVisible() && !callout_button.second->isInit())
            callout_button.second->renderSliderQuads(open_gl, animate);
    }


    editing_rotary_amount_quad_.render(open_gl, animate);
    editing_linear_amount_quad_.render(open_gl, animate);


    drawModulationDestinations(open_gl);

    drawCurrentModulator(open_gl);

    drawDraggingModulation(open_gl); // draw active drag icon
//
//  juce::Colour first_color = findColour(Skin::kWidgetPrimary1, true);
//  juce::Colour second_color = findColour(Skin::kWidgetPrimary2, true);
//
//  modulation_source_meters_->setAdditiveBlending(second_color.getBrightness() > 0.5f);
//  modulation_source_meters_->setColor(second_color);
//  renderSourceMeters(open_gl, 1);
//  modulation_source_meters_->setAdditiveBlending(first_color.getBrightness() > 0.5f);
//  modulation_source_meters_->setColor(first_color);
//  renderSourceMeters(open_gl, 0);
//  updateSmoothModValues();
//
}

void ModulationManager::renderMeters(OpenGlWrapper& open_gl, bool animate) {
    if (!animate)
        return;

    ScopedLock lock (open_gl_critical_section_);
    int num_voices = 1;
//  if (num_voices_readout_)
//    num_voices = std::max<float>(0.0f, num_voices_readout_->value()[0]);

    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();


    for (auto& meter : meter_lookup_) {
        SynthSlider* slider = slider_model_lookup_[meter.first];
        juce::Colour color = slider->findColour(Skin::kRotaryArc);
        auto* viewport = slider->findParentComponentOfClass<juce::Viewport>();
        auto meter_group = rotary_meters_.find(viewport);

       if (meter_group != rotary_meters_.end() && meter_group->second != nullptr){
           meter_group->second->setColor(color);
           meter_group->second->setAltColor(color);
           meter_group->second->setModColor(color);
       }

        bool show = slider != nullptr && meter.second->isModulated() && allVisible(slider) && slider->isShowing();
        meter.second->setActive(show);

        if (show) {
            if (parent) {
                meter.second->clearStaticModulationAmount();
                float range = slider->getMaximum() - slider->getMinimum();
                float display_value = slider->getValue();

                for (auto* connection : parent->getSynth()->getDestinationConnections(meter.first)) {
                    if (connection != nullptr && !connection->isBypass()) {
                        display_value += connection->getCurrentBaseValue() * range;
                        float amount = connection->getCurrentBaseValue();
                        if (connection->isBipolar())
                            amount *= 2.0f;
                        meter.second->setStaticModulationAmount(amount, connection->isBipolar());
                    }
                }
                meter.second->setCurrentValue(display_value);
            }
            meter.second->updateDrawing(num_voices);
        }
    }

    OpenGlComponent::setViewPort(this, open_gl);
    for (auto& rotary_meter_group : rotary_meters_)
        rotary_meter_group.second->render(open_gl, animate);

    for (auto& linear_meter_group : linear_meters_)
        linear_meter_group.second->render(open_gl, animate);
}

void ModulationManager::renderSourceMeters(OpenGlWrapper& open_gl, int index) {
  int i = 0;
  float width = getWidth();
  float height = getHeight();
////  for (auto& mod_readout : modulation_source_readouts_) {
////    ModulationButton* button = modulation_buttons_[mod_readout.first];
////    float readout_value = mod_readout.second->value()[index];
////
////    float clamped_value = electrosynth::utils::clamp(readout_value, 0.0f, 1.0f);
////    if (!active_mod_values_[mod_readout.first] && !mod_readout.second->isClearValue(readout_value))
////      smooth_mod_values_[mod_readout.first].set(index, clamped_value);
////    float smooth_value = smooth_mod_values_[mod_readout.first][index];
////
////    juce::Rectangle<int> bounds = getLocalArea(button, button->getMeterBounds());
////    float left = 2.0f * ((bounds.getX() - 1.0f) / width) - 1.0f + kModSourceMeterBuffer;
////    float w = 2.0f * bounds.getWidth() / width - 2.0f * kModSourceMeterBuffer;
////    float h = 2.0f * bounds.getHeight() / height - 2.0f * kModSourceMeterBuffer;
////    float y = 1.0f - 2.0f * bounds.getY() / height - kModSourceMeterBuffer;
////    float y_center = y - h * (1.0f - clamped_value);
////    float smooth_y_center = y - h * (1.0f - smooth_value);
////
////    float top = std::min(y_center, smooth_y_center) - kModSourceMinRadius;
////    float bottom = std::max(y_center, smooth_y_center) + kModSourceMinRadius;
////
////    bool active = button->isActiveModulation() || button->hasAnyModulation();
////    if (w <= 0.0f || mod_readout.second->isClearValue(readout_value) || !showingInParents(button) || !active) {
////      left = -2.0f;
////      top = -2.0f;
////      bottom = -2.0f;
////    }
//
////    modulation_source_meters_->positionBar(i, left, top, w, bottom - top);
//
//    i++;
//  }

  if (modulation_source_meters_)
    modulation_source_meters_->render(open_gl, true);
}

void ModulationManager::updateSmoothModValues() {
  static constexpr float kTimeDecayScale = 60.0f;
  long long current_milliseconds = juce::Time::currentTimeMillis();
  long long delta_milliseconds = current_milliseconds - last_milliseconds_;
  last_milliseconds_ = current_milliseconds;

  float seconds = delta_milliseconds / 1000.0f;
  float decay = std::max(std::min(kModSmoothDecay * seconds * kTimeDecayScale, 1.0f), 0.0f);

//  for (auto& mod_readout : modulation_source_readouts_) {
//    float readout_value = mod_readout.second->value();
//    float clamped_value = electrosynth::utils::clamp(readout_value, 0.0f, 1.0f);
//    float smooth_value = smooth_mod_values_[mod_readout.first];
//    bool active = !mod_readout.second->isClearValue(readout_value);
//    active_mod_values_[mod_readout.first] = active;
//    if (active)
//      smooth_mod_values_[mod_readout.first] = electrosynth::utils::interpolate(smooth_value, clamped_value, decay);
//  }
}

void ModulationManager::destroyOpenGlComponents(juce::OpenGLContext& open_gl) {
  SynthSection::destroyOpenGlComponents(open_gl);

    drag_quad_.destroy(open_gl);
    drag_icon_.destroy(open_gl);
    modulation_expansion_box_->destroy(open_gl);
    mapping_mode_dim_quad_.destroy(open_gl);
    current_modulator_quad_.destroy(open_gl);

    //  modulation_source_meters_->destroy(open_gl);
    for (auto& rotary_destination_group : rotary_destinations_)
        rotary_destination_group.second->destroy(open_gl);

    for (auto& linear_destination_group : linear_destinations_)
        linear_destination_group.second->destroy(open_gl);

    for (auto& rotary_meter_group : rotary_meters_)
        rotary_meter_group.second->destroy(open_gl);

    for (auto& linear_meter_group : linear_meters_)
        linear_meter_group.second->destroy(open_gl);
}

void ModulationManager::showModulationAmountOverlay(ModulationAmountKnob* slider) {
  electrosynth::ModulationConnection* connection = getConnection(slider->index());
  if (connection == nullptr || !meter_lookup_.contains (connection->destination_name))
    return;

  ModulationMeter* meter = meter_lookup_[connection->destination_name].get();
  if (!meter->destination()->isShowing())
    return;

  if (meter->isRotary()) {
      editing_rotary_amount_quad_.setTargetComponent(meter);
      editing_rotary_amount_quad_.setAdditive(false);
      meter->setAmountQuadVertices(editing_rotary_amount_quad_);
      meter->setModulationAmountQuad(editing_rotary_amount_quad_, slider->getValue(), slider->isBipolar());

      editing_rotary_amount_quad_.setThickness(2.0f);
      editing_rotary_amount_quad_.setAlpha(1.0f);
      editing_rotary_amount_quad_.setActive(true);
  }

  else {
      editing_linear_amount_quad_.setTargetComponent(meter);
      editing_linear_amount_quad_.setAdditive(false);
      meter->setAmountQuadVertices(editing_linear_amount_quad_);
      meter->setModulationAmountQuad(editing_linear_amount_quad_, slider->getValue(), slider->isBipolar());

      editing_linear_amount_quad_.setAlpha(1.0f);
      editing_linear_amount_quad_.setActive(true);
  }
}

void ModulationManager::hideModulationAmountOverlay() {
  if (changing_hover_modulation_)
    return;

  editing_rotary_amount_quad_.setAlpha(0.0f);
  editing_linear_amount_quad_.setAlpha(0.0f);
}

void ModulationManager::hoverStarted(SynthSlider* slider) {
  if (changing_hover_modulation_)
    return;

  if (!enteringHoverValue())
    makeModulationsVisible(slider, true);

  ModulationAmountKnob* amount_knob = dynamic_cast<ModulationAmountKnob*>(slider);
  if (amount_knob)
  {
      DBG(amount_knob->getName() + juce::String((uint64)(void*)amount_knob));
      showModulationAmountOverlay (amount_knob);
  }
  else
    hideModulationAmountOverlay();
}

void ModulationManager::hoverEnded(SynthSlider* slider) {
  hideModulationAmountOverlay();
  //cant make the modulation go away on destinatino hover end becuase then you can't ever get to the modualtion
  //could iomplement with some sort of short timer ?
//  if (changing_hover_modulation_)
//      return;
//  makeModulationsVisible(slider, false);
}

void ModulationManager::menuFinished(SynthSlider* slider) {
  if (current_modulator_ && current_modulator_->isVisible())
    current_modulator_->grabKeyboardFocus();
}

void ModulationManager::modulationsChanged(const std::string& destination) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();

  hideUnusedHoverModulations();
  updateModulationSlotVisuals();
  SynthSlider* slider = slider_model_lookup_[destination];
  if (current_modulator_)
    makeCurrentModulatorAmountsVisible();
  else if (slider)
    makeModulationsVisible(slider, slider->isShowing());

  if (parent == nullptr)
    return;

  if (!meter_lookup_.contains (destination))
    return;
  
  int num_modulations = parent->getSynth()->getNumModulations(destination);
  meter_lookup_[destination]->setModulated(num_modulations);
  meter_lookup_[destination]->setVisible(num_modulations);
}

int ModulationManager::getModulationIndex(std::string source, std::string destination, int destination_slot) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  std::vector<electrosynth::ModulationConnection*> connections = parent->getSynth()->getDestinationConnections(destination);

  for (electrosynth::ModulationConnection* connection : connections) {
    if (connection->source_name == source
        && (destination_slot < 0 || connection->destination_slot == destination_slot))
      return connection->index_in_all_mods;
  }

  return -1;
}

int ModulationManager::getIndexForModulationSlider(juce::Slider* slider) {
  ModulationAmountKnob* amount_knob = dynamic_cast<ModulationAmountKnob*>(slider);
  if (amount_knob)
    return amount_knob->index();
  return -1;
}

electrosynth::ModulationConnection* ModulationManager::getConnectionForModulationSlider(juce::Slider* slider) {
  int index = getIndexForModulationSlider(slider);
  if (index < 0)
    return nullptr;

  while (aux_connections_to_from_.count(index))
    index = aux_connections_to_from_[index];

  return getConnection(index);
}

electrosynth::ModulationConnection* ModulationManager::getConnection(int index) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return nullptr;

  return parent->getSynth()->getModulationBank().atIndex(index);
}

electrosynth::ModulationConnection* ModulationManager::getConnection(const std::string& source, const std::string& dest,
                                                                     int destination_slot) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return nullptr;

  std::vector<electrosynth::ModulationConnection*> connections = parent->getSynth()->getSourceConnections(source);
  for (electrosynth::ModulationConnection* connection : connections) {
    if (connection->destination_name == dest
        && (destination_slot < 0 || connection->destination_slot == destination_slot))
      return connection;
  }

  return nullptr;
}

void ModulationManager::mouseDown(SynthSlider* slider) {
    // ignore clicks on modulation amount knobs (the sliders under the synth knobs)
  for (auto& amount_knob : modulation_icon_) {
    if (slider == amount_knob.get())
      return;
  }

  if (modulation_expansion_box_->isVisible())
    return;

    // if there is modulation connected to this slider, select that modulation source
  electrosynth::ModulationConnection* connection = getConnectionForModulationSlider(slider);
  if (connection && !connection->source_name.empty() && !connection->destination_name.empty())
    modulationSelected(modulation_buttons_[connection->source_name]);
  else {
    clearModulationSource();
    hideModulationAmountOverlay();
    makeModulationsVisible(slider, true);
  }
}

void ModulationManager::mouseUp(SynthSlider* slider) {
  if (current_modulator_ && current_modulator_->isVisible())
    current_modulator_->grabKeyboardFocus();

}

void ModulationManager::doubleClick(SynthSlider* slider) {
  changing_hover_modulation_ = false;
  electrosynth::ModulationConnection* connection = getConnectionForModulationSlider(slider);
//  if (connection)
//    removeModulation(connection->source_name, connection->destination_name);
//  setModulationAmounts();
//
//  if (current_modulator_ && current_modulator_->isVisible())
//    current_modulator_->grabKeyboardFocus();
}

void ModulationManager::beginModulationEdit(SynthSlider* slider) {
  changing_hover_modulation_ = true;
}


void ModulationManager::endModulationEdit(SynthSlider* slider) {
  changing_hover_modulation_ = false;
}

void ModulationManager::sliderValueChanged(juce::Slider* slider) {
  ModulationAmountKnob* amount_knob = dynamic_cast<ModulationAmountKnob*>(slider);
  if (amount_knob == nullptr)
    return;

  float value = slider->getValue();
  int index = getIndexForModulationSlider(slider);
  float scale = getAuxMultiplier(index);
  float scaled_value = value * scale;
  while (aux_connections_to_from_.count(index))
    index = aux_connections_to_from_[index];

	  electrosynth::ModulationConnection* connection = getConnection(index);
  if (connection == nullptr)
    return;

  bool bipolar = connection->isBipolar();
	  bool stereo = connection->isStereo();
	  bool bypass = connection->isBypass();
  connection->setScalingValue(value);
//
  setModulationValues(connection->source_name, connection->destination_name, scaled_value, bipolar, stereo, bypass,
                      connection->destination_slot);
	  updateModulationSlotVisuals();
  showModulationAmountOverlay(amount_knob);
//
  SynthSection::sliderValueChanged(modulation_icon_[index].get());
}

void ModulationManager::buttonClicked(juce::Button* button) {
    for (auto& callout_button : modulation_callout_buttons_) {
        if (button == callout_button.second.get()) {
            bool new_button = button != current_expanded_modulation_;
            hideModulationAmountCallout();
            if (new_button) showModulationAmountCallout(callout_button.first);
            return;
        }
}

  SynthSection::buttonClicked(button);
}

bool ModulationManager::connectModulation(
    std::string source, std::string destination, int destination_slot) {
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || source.empty() || destination.empty())
        return false;

    modifying_ = true;
    const bool connected = parent->connectModulation(source, destination, destination_slot);
    modifying_ = false;

    if (connected)
        modulationsChanged(destination);

    return connected;
}

void ModulationManager::removeModulation(std::string source, std::string destination, int destination_slot) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || source.empty() || destination.empty())
    return;

  electrosynth::ModulationConnection* connection = getConnection(source, destination, destination_slot);
  if (connection == nullptr) {
    return;
  }
  
  int index = connection->index_in_all_mods;
  if (aux_connections_from_to_.count(index)) {
    float current_value = 0.5; //connection->modulation_processor->currentBaseValue();
    int dest_index = aux_connections_from_to_[index];
    ModulationAmountKnob* modulation_amount = modulation_icon_[dest_index].get();
    removeAuxSourceConnection(index);
    float reset_value = current_value == 0.0f ? 1.0f : -current_value;
    modulation_amount->setValue(reset_value, dontSendNotification);
    modulation_amount->setValue(current_value * 2.0f, sendNotificationSync);
  }
  else
    removeAuxSourceConnection(index);

  modifying_ = true;
  parent->disconnectModulation(connection);
  updateModulationSlotVisuals();
  modulationsChanged(destination);
  modifying_ = false;
}

void ModulationManager::setModulationSliderValue(int index, float value) {
  modulation_icon_[index]->setValue(value, dontSendNotification);
  modulation_icon_[index]->redoImage();
}

void ModulationManager::setModulationSliderBipolar(int index, bool bipolar) {
  modulation_icon_[index]->setBipolar(bipolar);
}

void ModulationManager::setModulationSliderValues(int index, float value) {
  setModulationSliderValue(index, value);
  float from_value = value;
  int from_index = index;
  while (aux_connections_from_to_.count(from_index)) {
    from_index = aux_connections_from_to_[from_index];
    from_value *= 2.0f;
    setModulationSliderValue(from_index, from_value);
  }

  float to_value = value;
  int to_index = index;
  while (aux_connections_to_from_.count(to_index)) {
    to_index = aux_connections_to_from_[to_index];
    to_value *= 0.5f;
    setModulationSliderValue(to_index, to_value);
  }

  setModulationSliderScale(index);
}

void ModulationManager::setModulationSliderScale(int index) {
  int end_index = index;
  float scale = 1.0f;
  while (aux_connections_from_to_.count(end_index)) {
    end_index = aux_connections_from_to_[end_index];
    scale *= 2.0f;
  }

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

//  electrosynth::ModulationConnectionBank& bank = parent->getSynth()->getModulationBank();
//  electrosynth::ModulationConnection* connection = bank.atIndex(end_index);
//  if (!connection->destination_name.empty()) {
//    electrosynth::ValueDetails details = electrosynth::juce::Parameters::getDetails(connection->destination_name);
//    if (details.value_scale == electrosynth::ValueDetails::kLinear || details.value_scale == electrosynth::ValueDetails::kIndexed) {
//      float display_multiply = scale * (details.max - details.min);
//      modulation_icon_[index]->setDisplayMultiply(display_multiply);
//      return;
//    }
//  }

  modulation_icon_[index]->setDisplayMultiply(1.0f);
}

void ModulationManager::setModulationValues(std::string source, std::string destination,
                                            float amount, bool bipolar, bool stereo, bool bypass,
                                            int destination_slot) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || source.empty() || destination.empty())
    return;

  modifying_ = true;
//  parent->setModulationValues(source, destination, amount, bipolar, stereo, bypass);
  int index = getModulationIndex(source, destination, destination_slot);
//  parent->notifyModulationValueChanged(index);
  if (juce::isPositiveAndBelow(index, electrosynth::kMaxModulationConnections)) {
    electrosynth::ModulationConnection* connection = getConnection(index);
    if (connection != nullptr) {
      connection->setBipolar(bipolar);
      connection->setStereo(stereo);
      connection->setBypass(bypass);
      connection->setScalingValue(amount);
    }
    setModulationSliderValues(index, amount);
    setModulationSliderBipolar(index, bipolar);
  }
  updateModulationSlotVisuals();
//
  modifying_ = false;
}

void ModulationManager::initAuxConnections() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  for (int i = 0; i < electrosynth::kMaxModulationConnections; ++i) {
    modulation_icon_[i]->removeAux();
  }

  aux_connections_from_to_.clear();
  aux_connections_to_from_.clear();
//
  electrosynth::ModulationConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < electrosynth::kMaxModulationConnections; ++i) {
    electrosynth::ModulationConnection* connection = bank.atIndex(i);
    int index = connection->index_in_all_mods;

    if (modulation_amount_lookup_.count(connection->destination_name)) {
      int modulation_index = modulation_amount_lookup_[connection->destination_name]->index();
      addAuxConnection(index, modulation_index);
    }
  }
}

void ModulationManager::reset() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || modifying_)
    return;

  for (auto& meter : meter_lookup_) {
    int num_modulations = parent->getSynth()->getNumModulations(meter.first);
    meter.second->setModulated(num_modulations);
    meter.second->setVisible(num_modulations);
  }

  for (auto& button : modulation_buttons_)
    button.second->setActiveModulation(button.second->isActiveModulation());

  setModulationAmounts();
  initAuxConnections();
  updateModulationSlotVisuals();
}

void ModulationManager::hideUnusedHoverModulations() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || changing_hover_modulation_)
    return;

  electrosynth::ModulationConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < electrosynth::kMaxModulationConnections; ++i) {
    electrosynth::ModulationConnection* connection = bank.atIndex(i);
    int index = connection->index_in_all_mods;
    if (connection->source_name.empty() || connection->destination_name.empty())
      modulation_icon_[index]->hideImmediately();
    else {
      SynthSlider* slider = slider_model_lookup_[connection->destination_name];
      if (slider == nullptr || !slider->isShowing())
        modulation_icon_[index]->hideImmediately();
    }
  }
}

float ModulationManager::getAuxMultiplier(int index) {
  float mult = 1.0f;
  while (aux_connections_to_from_.count(index)) {
    index = aux_connections_to_from_[index];
    mult *= 0.5f;
  }

  return mult;
}

void ModulationManager::addAuxConnection(int from_index, int to_index) {
  if (from_index == to_index)
    return;

  aux_connections_to_from_[to_index] = from_index;
  aux_connections_from_to_[from_index] = to_index;
  std::string aux_name = "modulation_" + std::to_string(from_index + 1) + "_amount";
  modulation_icon_[to_index]->setAux(aux_name);
  updateModulationSlotVisuals();

}

void ModulationManager::removeAuxSourceConnection(int from_index) {
  if (aux_connections_from_to_.count(from_index) == 0)
    return;

  int to_index = aux_connections_from_to_[from_index];
  modulation_icon_[to_index]->removeAux();
  aux_connections_from_to_.erase(from_index);
  aux_connections_to_from_.erase(to_index);
  updateModulationSlotVisuals();
}

void ModulationManager::removeAuxDestinationConnection(int to_index) {
  if (aux_connections_to_from_.count(to_index) == 0)
    return;

  modulation_icon_[to_index]->removeAux();
  aux_connections_from_to_.erase(aux_connections_to_from_[to_index]);
  aux_connections_to_from_.erase(to_index);
  updateModulationSlotVisuals();
}

void ModulationManager::makeCurrentModulatorAmountsVisible() {
    for (auto& selected_slider : modulation_icon_)
        selected_slider->makeVisible(false);

    positionModulationAmountSliders();
}

ModulationAmountKnob* ModulationManager::getModulationAmountControl(const electrosynth::ModulationConnection* connection) const {
    if (connection == nullptr
        || !juce::isPositiveAndBelow(connection->index_in_all_mods, electrosynth::kMaxModulationConnections))
        return nullptr;

    return modulation_icon_[connection->index_in_all_mods].get();
}

void ModulationManager::syncModulationAmountControl(electrosynth::ModulationConnection* connection,
    ModulationAmountKnob* amount_knob) {
    if (connection == nullptr || amount_knob == nullptr)
        return;

    if (!amount_knob->hasAux()) {
        amount_knob->setValue(connection->getCurrentBaseValue(), dontSendNotification);
        amount_knob->redoImage();
    }

    amount_knob->setSource(connection->source_name);
    amount_knob->setBipolar(connection->isBipolar());
    amount_knob->setStereo(connection->isStereo());
    amount_knob->setBypass(connection->isBypass());
}

bool ModulationManager::placeModulationAmountInSlot(SynthSlider* destination,
    const electrosynth::ModulationConnection* connection, ModulationAmountKnob* amount_knob) {
    if (destination == nullptr || connection == nullptr || amount_knob == nullptr
      || !juce::isPositiveAndBelow(connection->destination_slot, SynthSlider::kNumModulationSlots))
        return false;

    auto* target = destination->getExtraModulationTarget(connection->destination_slot);
    if (target == nullptr) return false;

    const juce::Point<int> top_left = getLocalPoint(target, juce::Point<int>());
    amount_knob->setBounds(top_left.x, top_left.y, target->getWidth(), target->getHeight());
    amount_knob->setPopupPlacement(juce::BubbleComponent::below);
    amount_knob->setAlwaysOnTop(true);
    amount_knob->getQuadComponent()->setAlwaysOnTop(true);
    amount_knob->getImageComponent()->setAlwaysOnTop(true);
    amount_knob->getQuadComponent()->setVisible(false);
    amount_knob->getImageComponent()->setVisible(false);
    return true;
}

void ModulationManager::makeModulationsVisible(SynthSlider* destination, bool visible) {

    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (destination == nullptr || parent == nullptr || changing_hover_modulation_)
        return;

    std::string name = destination->getComponentID().toStdString();
    auto slider_iter = slider_model_lookup_.find(name);
    if (slider_iter == slider_model_lookup_.end() || slider_iter->second != destination)
        return;


    std::vector<electrosynth::ModulationConnection*> connections = parent->getSynth()->getDestinationConnections(name);
    int num_amount_controls = 0;

    for (electrosynth::ModulationConnection* connection : connections) {
        auto* amount_knob = getModulationAmountControl(connection);
        if (amount_knob == nullptr) continue;
        syncModulationAmountControl(connection, amount_knob);
        ++num_amount_controls;
  }

  int amount_control_width = size_ratio_ * 24.0f;
  juce::Rectangle<int> destination_bounds = getLocalArea(destination, destination->getLocalBounds());
  int x = destination_bounds.getRight();
  int y = destination_bounds.getBottom();
  int beginning_offset = amount_control_width * num_amount_controls / 2;
  int delta_x = 0;
  int delta_y = 0;

  juce::BubbleComponent::BubblePlacement placement = destination->getModulationPlacement();
  if (placement == juce::BubbleComponent::below) {
    x = destination_bounds.getCentreX() - beginning_offset;
    delta_x = amount_control_width;
  }
  else if (placement == juce::BubbleComponent::above) {
    x = destination_bounds.getCentreX() - beginning_offset;
    y = destination_bounds.getY() - amount_control_width;
    delta_x = amount_control_width;
  }
  else if (placement == juce::BubbleComponent::left) {
    x = destination_bounds.getX() - amount_control_width;
    y = destination_bounds.getCentreY() - beginning_offset;
    delta_y = amount_control_width;
  }
  else {
    y = destination_bounds.getCentreY() - beginning_offset;
    delta_y = amount_control_width;
  }

  for (electrosynth::ModulationConnection* connection : connections) {
    auto* amount_knob = getModulationAmountControl(connection);
    if (amount_knob == nullptr)
      continue;

    bool placed_in_slot = placeModulationAmountInSlot(destination, connection, amount_knob);
    if (!placed_in_slot) {
      amount_knob->setPopupPlacement(placement);
      amount_knob->setBounds(x, y, amount_control_width, amount_control_width);
      amount_knob->setAlwaysOnTop(false);
      amount_knob->getQuadComponent()->setAlwaysOnTop(false);
      amount_knob->getImageComponent()->setAlwaysOnTop(false);
      amount_knob->getQuadComponent()->setVisible(true);
      amount_knob->getImageComponent()->setVisible(true);
    }

    amount_knob->makeVisible(visible && (!placed_in_slot || allVisible(destination)));
    amount_knob->setAlpha(placed_in_slot ? 0.0f : 1.0f, true);
    amount_knob->redoImage();

    x += delta_x;
    y += delta_y;
  }
}

void ModulationManager::positionModulationAmountSliders() {
  for (const auto& [name, slider] : slider_model_lookup_)
    makeModulationsVisible(slider, slider != nullptr && slider->isShowing());
}

void ModulationManager::showModulationAmountCallout(const std::string& source) {
  static constexpr int kSliderWidth = 30;
  static constexpr int kPadding = 5;

  ModulationButton* modulation_button = modulation_buttons_[source];
  current_expanded_modulation_ = modulation_callout_buttons_[source].get();
  std::vector<ModulationAmountKnob*> amount_controls = current_expanded_modulation_->getSliders();

  int num_sliders = static_cast<int>(amount_controls.size());
  int columns = current_expanded_modulation_->getNumColumns(num_sliders);
  int rows = (num_sliders + columns - 1) / columns;
  int width = kSliderWidth * columns + 2 * kPadding;
  int height = kSliderWidth * rows + 2 * kPadding;
  juce::Rectangle<int> top_level_modulation_bounds = getLocalArea(modulation_button, modulation_button->getLocalBounds());
  int start_x = top_level_modulation_bounds.getX() + (modulation_button->getWidth() - width) / 2;
  start_x = std::min(getWidth() - width, std::max(0, start_x));
  int start_y = top_level_modulation_bounds.getBottom();
  start_y = std::min(getHeight() - height, start_y);

  modulation_expansion_box_->setVisible(true);
  modulation_expansion_box_->setAmountControls(amount_controls);
  modulation_expansion_box_->setBounds(start_x, start_y, width, height);
  modulation_expansion_box_->setRounding(findValue(Skin::kBodyRounding));
  modulation_expansion_box_->grabKeyboardFocus();

  int row = 0;
  int column = 0;
  for (ModulationAmountKnob* slider : amount_controls) {
    int x = column * kSliderWidth + kPadding;
    int y = height - (row + 1) * kSliderWidth - kPadding;
    slider->setBounds(start_x + x, start_y + y, kSliderWidth, kSliderWidth);
    slider->setVisible(true);
    slider->setMouseClickGrabsKeyboardFocus(false);
    slider->redoImage();
    slider->getQuadComponent()->setAlwaysOnTop(true);

    column++;
    if (column >= columns) {
      column = 0;
      row++;
    }
  }
}

void ModulationManager::hideModulationAmountCallout() {
  if (current_expanded_modulation_ == nullptr)
    return;

  std::vector<ModulationAmountKnob*> amount_controls = current_expanded_modulation_->getSliders();
  for (ModulationAmountKnob* slider : amount_controls) {
    slider->setVisible(false);
    slider->getQuadComponent()->setAlwaysOnTop(false);
  }

  modulation_expansion_box_->setVisible(false);
  current_expanded_modulation_ = nullptr;
}


bool ModulationManager::enteringHoverValue() {
  for (int i = 0; i < electrosynth::kMaxModulationConnections; ++i) {
    if (modulation_icon_[i] && modulation_icon_[i]->enteringValue())
      return true;
  }
  return false;
}

void ModulationManager::setModulationAmounts() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || modifying_)
    return;

  electrosynth::ModulationConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < electrosynth::kMaxModulationConnections; ++i) {
    electrosynth::ModulationConnection* connection = bank.atIndex(i);
    if (aux_connections_to_from_.count(i) == 0)
      setModulationSliderValues(i, connection->getCurrentBaseValue());

    bool bipolar = connection->isBipolar();
    bool stereo = connection->isStereo();
    bool bypass = connection->isBypass();

    modulation_icon_[i]->setBipolar(bipolar);
    modulation_icon_[i]->setStereo(stereo);
    modulation_icon_[i]->setBypass(bypass);
  }
}

void ModulationManager::setVisibleMeterBounds() {
  for (auto& meter : meter_lookup_) {
    SynthSlider* slider = slider_model_lookup_[meter.first];
    if (slider && slider->isShowing()) {
      juce::Rectangle<int> local_bounds = getLocalArea(slider, slider->getModulationMeterBounds());
      meter.second->setBounds(local_bounds);
    }
  }
}
