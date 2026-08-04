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

#include "mapping_manager.h"

#include "FullInterface.h"
#include "ModulationConnection.h"
#include "ParameterView/ParametersView.h"
#include "connection_slots.h"
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

    // recursively checks if a component and all its parents are visible before showing modulation on knobs
    bool allVisible(juce::Component* component) {
        if (component == nullptr || component->getParentComponent() == nullptr)
            return true;
        return component->isVisible() && allVisible(component->getParentComponent());
    }

    juce::String getConnectionSourceLabel(const juce::String& source_name) {
        juce::String prefix;
        if (source_name.startsWithIgnoreCase("env"))
            prefix = "Env ";
        else if (source_name.startsWithIgnoreCase("lfo"))
            prefix = "Lfo ";
        else if (source_name.startsWithIgnoreCase("vca") || source_name.containsIgnoreCase("master"))
            prefix = "Master ";
        else if (source_name.startsWithIgnoreCase("filt"))
            prefix = "Flt ";
        else if (source_name.startsWithIgnoreCase("delay"))
            prefix = "Dly ";
        else
        return prefix = "Noise ";

        juce::String digits;
        for (auto character : source_name) {
        if (juce::CharacterFunctions::isDigit(character))
          digits += character;
        }

        return prefix + (digits.isNotEmpty() ? digits : "#");
    }

    juce::Colour getConnectionSourceColor(const juce::String& source_name) {
        if (source_name.startsWithIgnoreCase("env"))
            return ShaderColors::kEnvelopeTextColor;
        if (source_name.startsWithIgnoreCase("lfo"))
            return ShaderColors::kLfoTextColor;
        if (source_name.startsWithIgnoreCase("delay") || source_name.startsWithIgnoreCase("filt"))
            return ShaderColors::kEffectTextColor;
        if (source_name.startsWithIgnoreCase("vca") || source_name.containsIgnoreCase("master"))
            return ShaderColors::kMasterEnvelopeTextColor;
        return ShaderColors::kNoise;
    }
}

// custom UI class inheriting from OpenGlToggleButton. When a modulation source has too many connections, instead of displaying
// each one individually next to the button, this appears instead and acts as a collapsed popup
class ExpandConnectionButton : public OpenGlToggleButton {
  public:
    ExpandConnectionButton() : OpenGlToggleButton("expand connection"),
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
        while (columns * static_cast<int> (height_width_ratio * columns) < num_sliders)
            columns++;

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
    juce::Colour colors_[electrosynth::kMaxConnections];
    OpenGlQuad amount_quad_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExpandConnectionButton)
};

// represents (wraps) a synthslider, tracks whether it is active/already modulated, computes the visual bounds for the
// drag-over highlight, stores the OpenGl quad index used when drawing destination overlays, handles the three small boxes
// under knobs (extra modulation target boxes)
class ConnectionDestination : public juce::Component {
  public:
    ConnectionDestination(SynthSlider* source) : destination_slider_(source), margin_(0), index_(0),
                                                 size_multiple_(0.3f), active_(false), rectangle_(false), rotary_(true) {
      setComponentID(source->getComponentID());
    }

    ~ConnectionDestination() override = default;

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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectionDestination)
};

// creates the UI knob that controls how much modulation is applied to another slider
ModulationAmountKnob::ModulationAmountKnob(juce::String name, int index, const ValueTree &v) :
SynthSlider(name), color_component_(nullptr), index_(index) {
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
  current_source_ = false;
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

        if (parent_ != nullptr)
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
        listener->setConnectionBypass(this, bypass_);

}

void ModulationAmountKnob::handleModulationMenuCallback(int result) {
    if (result == kDisconnect) {
        for (Listener* listener : listeners_)
            listener->disconnectConnection(this);
    }
    else if (result == kToggleBypass)
        toggleBypass();
    else if (result == kToggleBipolar) {
        bipolar_ = !bipolar_;
        for (Listener* listener : listeners_)
            listener->setConnectionBipolar(this, bipolar_);
    }
    else if (result == kToggleStereo) {
        stereo_ = !stereo_;
        for (Listener* listener : listeners_)
            listener->setConnectionStereo(this, stereo_);
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

void ModulationAmountKnob::setCurrentSource(bool current) {
    if (current_source_ == current)
        return;

    setColour(Skin::kRotaryArc, findColour(Skin::kModulationMeterControl, true));
    current_source_ = current;
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
  return getConnectionSourceLabel(source_name_);
}

juce::Colour ModulationAmountKnob::getSourceColor() const {
  return getConnectionSourceColor(source_name_);
}

MappingManager::MappingManager(ValueTree &tree, SynthBase* base) :
        SynthSection("modulation_manager"), drag_quad_(Shaders::kRingFragment), drag_icon_("modulation_drag_icon"),
        current_quad_(Shaders::kRoundedRectangleBorderFragment),
        mapping_mode_dim_quad_(Shaders::kColorFragment, "modulation_mapping_mode_dim"),
        editing_rotary_amount_quad_(Shaders::kRotaryModulationFragment),
        editing_linear_amount_quad_(Shaders::kLinearModulationFragment), modifying_(false), dragging_(false),
        changing_hover_(false),component_update_pending_(false), current_modulator_(nullptr),
        expansion_box_(std::make_shared<SlotExpansionBox>()), state_(tree) {

    current_quad_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
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

    addOpenGlComponent(expansion_box_);
    expansion_box_->setVisible(false);
    expansion_box_->setWantsKeyboardFocus(true);
    expansion_box_->addListener(this);
    expansion_box_->setAlwaysOnTop(true);

    setSkinOverride(Skin::kModulationDragDrop);

    current_source_ = nullptr;
    current_expanded_ = nullptr;
    temporarily_set_destination_ = nullptr;
    temporarily_set_synth_slider_ = nullptr;
    temporarily_set_hover_slider_ = nullptr;
    temporarily_set_slot_ = -1;
    temporarily_set_bipolar_ = false;
    setInterceptsMouseClicks(false, true);

    destinations_ = std::make_unique<juce::Component>();
    destinations_->setInterceptsMouseClicks(false, true);
    addChildComponent(destinations_.get());

    mapping_mode_dim_quad_.setTargetComponent(this);
    mapping_mode_dim_quad_.setColor(juce::Colours::black);
    mapping_mode_dim_quad_.setAlpha(0.0f);
    mapping_mode_dim_quad_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);


    electrosynth::ConnectionBank & bank = base->getModulationBank();
    for (int i = 0; i < electrosynth::kMaxConnections; ++i) {
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

// Endpoint organization and maintenance methods *****************************************************************  Endpoint organization and maintenance methods
void MappingManager::registerEndpoint(EndpointArrowComponent& endpoint) {
    if (!endpoint.hasEndpoint()) return;

    const auto address = endpoint.getEndpoint().address;
    const auto endpoint_key = getEndpointKey(address);

    auto existing = mapping_endpoints_.find(endpoint_key); // clear listener from old recreated component
    if (existing != mapping_endpoints_.end()) {
        if (auto* oldComponent = existing->second.component.getComponent())
            oldComponent->removeMouseListener(this);
    }

    endpoint.addMouseListener(this, false);
    mapping_endpoints_[endpoint_key] = {
        .component = &endpoint
    };

    if (auto* slots = endpoint.getConnectionSlots())
        slots->addListener(this);

    updateConnectionSlots();
}

void MappingManager::unregisterEndpoint(const EndpointArrowComponent& endpoint) {
    if (auto* slots = endpoint.getConnectionSlots())
        slots->removeListener(this);

    unregisterEndpoint(endpoint.getEndpoint().address);
}

void MappingManager::unregisterEndpoint(const electrosynth::EndpointAddress& address) {
    const auto endpoint_key = getEndpointKey(address);
    const auto found = mapping_endpoints_.find(endpoint_key);

    if (found == mapping_endpoints_.end()) return;

    const bool removing_drag_source = endpoint_drag_source_.has_value()
        && endpoint_drag_source_->matches(address);
    const bool removing_drag_destination = endpoint_drag_destination_.has_value()
        && endpoint_drag_destination_->matches(address);

    if (removing_drag_source || removing_drag_destination) {
        clearEndpointDestinationVisuals();
        drag_icon_.setVisible(false);
        drag_icon_.setActive(false);
        endpoint_drag_source_.reset();
        endpoint_drag_source_component_ = nullptr;
        endpoint_drag_destination_.reset();
        endpoint_drag_destination_component_ = nullptr;
    }

    if (auto* component = found->second.component.getComponent())
        component->removeMouseListener(this);

    mapping_endpoints_.erase(found);
    std::erase_if(connection_records_, [&address](const auto& connection) {
        return connection.source.matches(address) || connection.destination.matches(address);
    });
    updateConnectionSlots();
}

juce::String MappingManager::getEndpointKey(const electrosynth::EndpointAddress& address) {
    return juce::String(static_cast<int>(address.type)) + ":" + address.nodeId + ":" + address.endpointId
        + ":" + juce::String(static_cast<int>(address.direction));
}

RegisteredMappingEndpoint* MappingManager::getRegisteredMappingEndpoint(juce::Component* component) {
    for (auto& [key, endpoint] : mapping_endpoints_) {
        if (endpoint.component.getComponent() == component)
            return &endpoint;
    }

    return nullptr;
}

RegisteredMappingEndpoint* MappingManager::getRegisteredMappingEndpoint(const electrosynth::EndpointAddress& address) {
    const auto found = mapping_endpoints_.find(getEndpointKey(address));
    if (found == mapping_endpoints_.end()) return nullptr;
    return &found->second;
}

// Generic Endpoint mouse handlers  **********************************************************************************************  Generic Endpoint mouse handlers
void MappingManager::mouseDown(const juce::MouseEvent& event) {
    auto* registered_endpoint = getRegisteredMappingEndpoint(event.eventComponent);
    auto* endpoint = registered_endpoint != nullptr
        ? registered_endpoint->component.getComponent()
        : nullptr;
    if (endpoint == nullptr
        || endpoint->getEndpoint().address.direction != electrosynth::EndpointDirection::Source)
        return;

    clearEndpointDestinationVisuals();
    endpoint_drag_destination_.reset();
    endpoint_drag_destination_component_ = nullptr;
    endpoint_drag_source_ = endpoint->getEndpoint().address;
    endpoint_drag_source_component_ = endpoint;

    const auto& address = endpoint->getEndpoint().address;

    if (address.type == electrosynth::ConnectionType::Modulation) {
        auto* button = dynamic_cast<ConnectionButton*>(endpoint);
        if (button == nullptr)
            return;
        // Existing modulation selection behavior.
        connectionSelected(button);
        return;
        }

    // Existing audio setup remains below.
    clearEndpointDestinationVisuals();

    mouse_drag_position_ = getLocalPoint(event.eventComponent, event.getPosition());

    // visuals
    drag_icon_.setShape(Paths::rightArrow());
    updateEndpointDestinationVisuals();
    positionEndpointDragIcon();
}

void MappingManager::mouseDrag(const juce::MouseEvent& event) {
    if (!endpoint_drag_source_.has_value() || endpoint_drag_source_component_ == nullptr)
        return;

    if (endpoint_drag_source_->type == electrosynth::ConnectionType::Modulation)
    {
        auto* button = dynamic_cast<ConnectionButton*>(endpoint_drag_source_component_.getComponent());

        if (button == nullptr)
            return;

        const auto sourceEvent = event.getEventRelativeTo(button);
        if (!dragging_) {
            if (button->getLocalBounds().contains(sourceEvent.getPosition()))
                return;

            startDestinationMap(button, sourceEvent);
        }

        if (dragging_)
            mappingDragged(sourceEvent);
        return;
    }

    mouse_drag_position_ = getLocalPoint(event.eventComponent, event.getPosition());
    positionEndpointDragIcon();

    auto* destination = findEndpointAt(mouse_drag_position_);
    auto* previous = endpoint_drag_destination_component_.getComponent();

    // are we hovering over an actual destination?
    auto* next = destination != nullptr ?
        destination->component.getComponent() : nullptr;

    // if we are, is this new destination different than the last?
    if (previous != next) {
        if (previous != nullptr)
            previous->setDragTarget(false); // clear previous drag target

        if (next != nullptr)
            next->setDragTarget(true);  // set new drag target
    }

    if (next == nullptr) {
        endpoint_drag_destination_.reset();
        endpoint_drag_destination_component_ = nullptr;
        return;
    }

    endpoint_drag_destination_ = next->getEndpoint().address;
    endpoint_drag_destination_component_ = next;
}

void MappingManager::mouseUp(const juce::MouseEvent& event) {

    if (endpoint_drag_source_ && endpoint_drag_source_->type == electrosynth::ConnectionType::Modulation) {

        auto* button = dynamic_cast<ConnectionButton*>(endpoint_drag_source_component_.getComponent());

        if (dragging_)
            endDestinationMap();
        else if (button != nullptr)
            connectionClicked(button);

        endpoint_drag_source_.reset();
        endpoint_drag_source_component_ = nullptr;
        endpoint_drag_destination_.reset();
        endpoint_drag_destination_component_ = nullptr;
        return;
    }

    // if valid, connect endpoints
    if (endpoint_drag_source_.has_value() && endpoint_drag_destination_.has_value()) {
        connectEndpoints(*endpoint_drag_source_, *endpoint_drag_destination_);
    }

    // clean up
    clearEndpointDestinationVisuals();
    drag_icon_.setVisible(false);
    drag_icon_.setActive(false);

    endpoint_drag_source_.reset();
    endpoint_drag_source_component_ = nullptr;

    endpoint_drag_destination_.reset();
    endpoint_drag_destination_component_ = nullptr;
}

// Making connection helpers  ************************************************************************************************************ Making connection helpers
bool MappingManager::endpointsAreCompatible(const electrosynth::EndpointAddress& source, const electrosynth::EndpointAddress& destination) const {
    if (!source.isValid() || !destination.isValid())
        return false;

    if (source.direction != electrosynth::EndpointDirection::Source ||
        destination.direction != electrosynth::EndpointDirection::Destination)
        return false;

    if (source.type != destination.type)
        return false;

    if (source.type == electrosynth::ConnectionType::Audio) {
        return source.audioDomain == destination.audioDomain && source.nodeId != destination.nodeId;
    }

    return true;
}

RegisteredMappingEndpoint* MappingManager::findEndpointAt(juce::Point<int> managerPosition) {
    const auto screen_position = localPointToGlobal(managerPosition);

    for (auto& [key, endpoint] : mapping_endpoints_) {
        auto* component = endpoint.component.getComponent();
        if (component == nullptr || !component->isShowing() || !endpoint_drag_source_.has_value() ||
            !endpointsAreCompatible(*endpoint_drag_source_, component->getEndpoint().address))
            continue;

        const auto local_position = component->getLocalPoint(nullptr, screen_position);
        if (component->reallyContains(local_position, false))
            return &endpoint;
    }
    return nullptr;
}

bool MappingManager::connectEndpoints (const electrosynth::EndpointAddress& source, const electrosynth::EndpointAddress& destination)
{
    if (!endpointsAreCompatible(source, destination)) return false;

    auto* destinationEndpoint = getRegisteredMappingEndpoint(destination);
    if (destinationEndpoint == nullptr) return false;

    auto* destination_component = destinationEndpoint->component.getComponent();
    if (destination_component == nullptr) return false;

    const int capacity = destination_component->getEndpoint().capabilities.maxIncomingConnections;
    const auto connectionsToDestination = [&] (const electrosynth::ConnectionRecord& record) {
        return record.destination.matches(destination);
    };

    // does this connection already exist?
    const auto duplicate = std::find_if(connection_records_.begin(),connection_records_.end(),
        [&](const auto& record) {
            return record.source.matches(source) && record.destination.matches(destination);
        });
    if (duplicate != connection_records_.end()) return true;

    const int existingConnectionsCount = static_cast<int>(std::count_if(connection_records_.begin(), connection_records_.end(),
        connectionsToDestination));

    if (capacity <= 0 || existingConnectionsCount >= capacity) {
        return false;
    }

    electrosynth::ConnectionRecord connection {
        .id = electrosynth::createConnectionRecordId(),
        .type = source.type,
        .source = source,
        .destination = destination
    };

    auto* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr || !parent->connect(connection))
        return false;

    connection_records_.push_back(std::move(connection));
    updateConnectionSlots();
    return true;
}

void MappingManager::updateConnectionSlots()
{
    const auto get_label = [](const juce::String& label) {
        const auto abbreviate = [&label](const juce::String& full_label, const juce::String& new_label) -> std::optional<juce::String> {
            if (!label.startsWithIgnoreCase (full_label)) return std::nullopt;
            const auto suffix = label.substring(full_label.length()).trimStart();
            return suffix.isEmpty() ? new_label : new_label + " " + suffix;
        };
        if (auto result = abbreviate("Oscillator", "osc")) return *result;
        if (auto result = abbreviate("Filter", "flt")) return *result;
        if (auto result = abbreviate("String", "str")) return *result;
        if (auto result = abbreviate("Soft Clip", "clp")) return *result;
        if (auto result = abbreviate("Delay", "dly")) return *result;
        if (auto result = abbreviate("Noise", "ns")) return *result;
        if (auto result = abbreviate("Lane", "ln")) return *result;
        return label;
    };

    for (auto& [key, registered_endpoint] : mapping_endpoints_) {
        auto* port = registered_endpoint.component.getComponent();
        if (port == nullptr) continue;

        const auto& address = port->getEndpoint().address;
        if (address.type != electrosynth::ConnectionType::Audio) continue;

        std::vector<ConnectionSlotData> slots_for_port;
        for (const auto& connection : connection_records_) {
            if (connection.type != electrosynth::ConnectionType::Audio) continue;
            const auto& endpoint = address.direction == electrosynth::EndpointDirection::Destination ? connection.destination : connection.source;
            if (!endpoint.matches(address)) continue;
            const auto& peer_address = address.direction == electrosynth::EndpointDirection::Destination ? connection.source : connection.destination;
            auto* peer_endpoint = getRegisteredMappingEndpoint(peer_address);
            if (peer_endpoint == nullptr) continue;
            auto* peer = peer_endpoint->component.getComponent();
            if (peer == nullptr) continue;

            auto* destination_endpoint = getRegisteredMappingEndpoint(connection.destination);
            auto* destination_component = destination_endpoint != nullptr ? destination_endpoint->component.getComponent() : nullptr;
            if (destination_component == nullptr) continue;

            auto* owner = peer->getParentComponent();
            const auto full_label = owner != nullptr && owner->getName().isNotEmpty() ? owner->getName() : peer->getName();

            slots_for_port.push_back({
                .connectionId = connection.id,
                .peer = peer_address,
                .label = get_label(full_label),
                .colour = peer->findColour(Skin::kWidgetPrimary1, true),

                .hasAmount = destination_component->getEndpoint().capabilities.hasAmount,
                .hasBipolar = destination_component->getEndpoint().capabilities.hasBipolar,
                .hasStereo = destination_component->getEndpoint().capabilities.hasStereo,
                .amount = connection.amount,
                .bipolar = connection.bipolar,
                .bypass = connection.bypass,
                .stereo = connection.stereo
            });
        }
        if (auto* slots = port->getConnectionSlots()) slots->setConnections (std::move(slots_for_port));
    }
}

// Drag/drop visuals ***************************************************************************************************************************** Drag/drop visuals

bool MappingManager::isMappingMode() const {

    const bool endpoint_mapping = endpoint_drag_source_.has_value() && endpoint_drag_source_component_ != nullptr
    && endpoint_drag_source_->type == electrosynth::ConnectionType::Audio;
    const bool modulation_mapping = dragging_ && current_modulator_ != nullptr;

    return modulation_mapping || endpoint_mapping;
}

void MappingManager::updateEndpointDestinationVisuals() {
    if (!endpoint_drag_source_) return;
    for (auto& [key, endpoint] : mapping_endpoints_) {
        if (auto* arrow = endpoint.component.getComponent())
            arrow->setMappingTarget(endpointsAreCompatible(*endpoint_drag_source_, arrow->getEndpoint().address));
    }
}

void MappingManager::clearEndpointDestinationVisuals() {
    for (auto& [key, endpoint] : mapping_endpoints_) {
        if (auto* arrow = endpoint.component.getComponent()) {
            arrow->setMappingTarget(false);
            arrow->setDragTarget(false);
        }
    }
}
// position arrow over mouse_drag_position_
void MappingManager::positionEndpointDragIcon() {
    if (!endpoint_drag_source_ || endpoint_drag_source_component_ == nullptr || getWidth() <= 0) return;

    const int arrow_size = static_cast<int>(std::round(0.03f * getWidth()));

    drag_icon_.setBounds(mouse_drag_position_.x - arrow_size / 2, mouse_drag_position_.y - arrow_size / 2, arrow_size, arrow_size);
    drag_icon_.setColor(endpoint_drag_source_component_->getArrowColor());
    drag_icon_.setActive(true);
    drag_icon_.setVisible(true);
    drag_icon_.redrawImage(true);
}

// make valid destinations render [place this call after the dim overlay so these destinations are bright)
void MappingManager::drawEndpointDestinations(OpenGlWrapper& openGl) {
    if (!endpoint_drag_source_) return;
    for (auto& [key, endpoint] : mapping_endpoints_) {
        auto* arrow = endpoint.component.getComponent();
        if (arrow == nullptr
            || !endpointsAreCompatible(*endpoint_drag_source_, arrow->getEndpoint().address))
            continue;

        arrow->render(openGl, true);
    }
}

// connection slider callbacks *************************************************************************************************************** connection slider callbacks

void MappingManager::connectionSlotClicked(const ConnectionSlotData& connection, const juce::MouseEvent& event)  {
    DBG("Clicked connection: " + connection.connectionId);

    if (!event.mods.isPopupMenu())
        return;

    PopupItems options;
    options.addItem (kRemoveConnection, "Remove connection");
    options.addItem(kToggleConnectionBypass, connection.bypass ? "Unbypass" : "Bypass");

    if (connection.hasBipolar) {
        options.addItem(kToggleConnectionBipolar, connection.bipolar ? "Make unipolar" : "Make bipolar");
    }

    if (connection.hasStereo) {
        options.addItem(kToggleConnectionStereo, connection.stereo ? "Make mono" : "Make stereo");
    }

    const auto connectionId = connection.connectionId;
    auto* source_component = event.eventComponent;

    showPopupSelector(source_component, event.getPosition(), options, [this, connectionId](int result) {
        handleConnectionMenuResult(connectionId, result);
    });
}

void MappingManager::handleConnectionMenuResult(const juce::String& connectionId, int result) {
    if (result == kRemoveConnection) {
        removeConnectionRecord(connectionId);
        return;
    }

    auto audioConnectionRecord = std::find_if(connection_records_.begin(), connection_records_.end(),
        [&connectionId](const electrosynth::ConnectionRecord& connection) {
            return connection.id == connectionId;
        });

    if (audioConnectionRecord != connection_records_.end()) {
        switch (result)
        {
            case kToggleConnectionBypass:
                audioConnectionRecord->bypass = !audioConnectionRecord->bypass;
                break;

            case kToggleConnectionBipolar:
                audioConnectionRecord->bipolar = !audioConnectionRecord->bipolar;
                break;

            case kToggleConnectionStereo:
                audioConnectionRecord->stereo = !audioConnectionRecord->stereo;
                break;

            default:
                return;
        }

        updateConnectionSlots();
        return;
    }

    auto* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return;

    auto& modulationBank = parent->getSynth()->getModulationBank();
    for (int index = 0; index < electrosynth::kMaxConnections; ++index) {
        auto* modulationConnection = modulationBank.atIndex(index);
        if (modulationConnection == nullptr
            || juce::String(modulationConnection->uuid) != connectionId)
            continue;

        bool bipolar = modulationConnection->isBipolar();
        bool stereo = modulationConnection->isStereo();
        bool bypass = modulationConnection->isBypass();

        switch (result) {
            case kToggleConnectionBypass:
                bypass = !bypass;
                break;
            case kToggleConnectionBipolar:
                bipolar = !bipolar;
                break;
            case kToggleConnectionStereo:
                stereo = !stereo;
                break;
            default:
                return;
        }

        setConnectionValues(modulationConnection->source_name,
                            modulationConnection->destination_name,
                            modulationConnection->getCurrentBaseValue(),
                            bipolar, stereo, bypass,
                            modulationConnection->destination_slot);
        return;
    }
}

void MappingManager::removeConnectionRecord(const juce::String& connectionId)
{
    const auto removed = std::erase_if(connection_records_, [&connectionId](const auto& connection) {
        return connection.id == connectionId;
    });

    if (removed > 0) {
        updateConnectionSlots();
        return;
    }

    auto* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr) return;

    auto& modulation_bank = parent->getSynth()->getModulationBank();
    for (int index = 0; index < electrosynth::kMaxConnections; ++index){
        auto* connection = modulation_bank.atIndex(index);
        if (connection == nullptr || juce::String(connection->uuid) != connectionId) continue;

        const auto source = connection->source_name;
        const auto destination = connection->destination_name;
        const int destination_slot = connection->destination_slot;

        removeMapping(source, destination, destination_slot);
        return;
    }
}

void MappingManager::connectionAmountChanged(const ConnectionSlotData& connection, float amount)  {
    // audio connections first
    auto found_audio = std::find_if(connection_records_.begin(), connection_records_.end(),
        [&connection](const auto& record) {
            return record.id == connection.connectionId;
        });

    if (found_audio != connection_records_.end()) {
        found_audio->amount = amount;
        updateConnectionSlots();
        return;
    }

    // now onto modulation connection
    auto* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return;

    auto& bank = parent->getSynth()->getModulationBank();

    for (int index = 0; index < electrosynth::kMaxConnections; ++index) {
        const auto* current_connection = bank.atIndex(index);

        if (current_connection == nullptr || juce::String(current_connection->uuid) != connection.connectionId) {
            continue;
            }

        setConnectionValues(
            current_connection->source_name,
            current_connection->destination_name,
            amount,
            current_connection->isBipolar(),
            current_connection->isStereo(),
            current_connection->isBypass(),
            current_connection->destination_slot);

        return;
    }
}
















void MappingManager::createMappingMeter(SynthSlider* slider, OpenGlMultiQuad* quads, int index) {
  std::string name = slider->getComponentID().toStdString();

  std::unique_ptr<ModulationMeter> meter = std::make_unique<ModulationMeter>(slider, quads, index);
  addChildComponent(meter.get());
  meter->setName(name);
  meter->setBounds(getLocalArea(slider, slider->getLocalBounds()));
  meter_lookup_[name] = std::move(meter);
}

void MappingManager::createMappingSlider(std::string name, SynthSlider* slider) {

    std::unique_ptr<ConnectionDestination> destination = std::make_unique<ConnectionDestination>(slider);
    destinations_->addAndMakeVisible(destination.get());

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

MappingManager::~MappingManager() {
    for (auto& [key, endpoint] : mapping_endpoints_)
        if (auto* component = endpoint.component.getComponent())
            component->removeMouseListener(this);
}

void MappingManager::resized() {
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



  destinations_->setBounds(getLocalBounds());

  updateMappingMeterLocations();

  juce::Colour meter_control = findColour(Skin::kModulationMeterControl, true);
  current_quad_.setColor(meter_control);
  drag_quad_.setColor(meter_control);
  drag_quad_.setThumbColor(meter_control);
  drag_quad_.setAltColor(findColour(Skin::kWidgetBackground, true));


    expansion_box_->setColor(findColour(Skin::kBody, true));
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
  clearConnectionSource();
}

void MappingManager::updateMappingMeterLocations() {
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

void MappingManager::connectionAmountChanged(SynthSlider* slider) {
  std::string slider_name = slider->getComponentID().toStdString();
  std::string source_name = current_modulator_->getComponentID().toStdString();
  setConnectionValues(source_name, slider_name,
                      slider->getModulationAmount(), slider->isModulationBipolar(),
                      slider->isModulationStereo(), slider->isModulationBypassed());
  modulation_buttons_[source_name]->repaint();
}

void MappingManager::connectionRemoved(SynthSlider* slider) {
  std::string slider_name = slider->getComponentID().toStdString();
  std::string source_name = current_modulator_->getComponentID().toStdString();

  removeMapping(source_name, slider_name);
  modulation_buttons_[source_name]->repaint();
}

void MappingManager::connectionSelected(ConnectionButton* source) {

  current_modulator_ = source;
  for (auto& hover_slider : modulation_icon_)
    hover_slider->makeVisible(false);
  makeCurrentConnectionAmountsVisible();
  setConnectionAmounts();
}

void MappingManager::connectionClicked(ConnectionButton* source) {
  hideUnusedHoverModulations();
}

bool MappingManager::hasFreeConnection() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  electrosynth::ConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < electrosynth::kMaxConnections; ++i) {
    electrosynth::Connection* connection = bank.atIndex(i);
    if (connection->source_name.empty() && connection->destination_name.empty())
      return true;
  }

  return false;
}

void MappingManager::scheduleComponentUpdate()
{
  if (component_update_pending_)
    return;                          // coalesce repeated add/remove triggers
  component_update_pending_ = true;
  juce::Component::SafePointer<MappingManager> safe_this(this);
  juce::MessageManager::callAsync([safe_this]() {
    if (safe_this == nullptr)
      return;                        // manager destroyed before this turn ran
    safe_this->component_update_pending_ = false;  // reset before rebuild so
                                     // componentAdded()'s not-ready retry logic still works
    safe_this->componentAdded();
  });
}

void MappingManager::componentAdded() {
  FullInterface* full = findParentComponentOfClass<FullInterface>();
  if (full == nullptr || !full->open_gl_.context.isAttached() || full->open_gl_.shaders == nullptr) {
    if (!component_update_pending_) {
      component_update_pending_ = true;
      juce::Component::SafePointer<MappingManager> safe_this(this);
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

        auto contains_current_modulator = [&mod_buttons] (ConnectionButton* button) {
            if (button == nullptr)
                return false;
            for (const auto& modulation_button : mod_buttons)
                if (modulation_button.second == button)
                    return true;
            return false;
        };

        if (!contains_current_modulator(current_source_))
            current_source_ = nullptr;
        current_expanded_ = nullptr;
        expansion_box_->setVisible(false);


        dragging_ = false;
        current_source_ = nullptr;
        current_modulator_ = nullptr;
        temporarily_set_destination_ = nullptr;
        temporarily_set_synth_slider_ = nullptr;
        temporarily_set_hover_slider_ = nullptr;
        temporarily_set_slot_ = -1;
        destinations_->setVisible(false);


        rotary_destinations_.clear();
        rotary_meters_.clear();
        linear_destinations_.clear();
        linear_meters_.clear();
        destination_lookup_.clear();
        all_destinations_.clear();
        modulation_buttons_.clear();
        callout_buttons_.clear();
        meter_lookup_.clear();
        num_linear_meters.clear();
        num_rotary_meters.clear();
        modulation_buttons_ = mod_buttons;
        for (auto& modulation_button : modulation_buttons_) {
            if (!modulation_button.second->hasEndpoint()) {
                modulation_button.second->addListener(this);
            }
            callout_buttons_[modulation_button.first] = std::make_unique<ExpandConnectionButton>();
            addChildComponent(callout_buttons_[modulation_button.first].get());
            // addOpenGlComponent(modulation_callout_buttons_[modulation_button.first]->getGlComponent());
            callout_buttons_[modulation_button.first]->addListener(this);
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
                    num_linear_meters[viewport] += has_slots ? SynthSlider::kNumSlots : 1;
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
                createMappingMeter(slider.second, rotary_meters_[viewport].get(), index);
            }
            else if (linear) {
                int index = num_linear_meters[viewport] - 1;
                num_linear_meters[viewport] = index;
                createMappingMeter (slider.second, linear_meters_[viewport].get(), index);
            }

            slider.second->addSliderListener (this);
            createMappingSlider (name, slider.second);
        }
    }

    updateSlotVisuals();
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

void MappingManager::drawMappingMode(OpenGlWrapper& open_gl) {
    if (!isMappingMode()) {
        mapping_mode_dim_quad_.setAlpha(0.0f);
        return;
    }
    mapping_mode_dim_quad_.setTargetComponent(this);
    mapping_mode_dim_quad_.setColor(juce::Colours::black);
    mapping_mode_dim_quad_.setAlpha(0.45f);
    mapping_mode_dim_quad_.render(open_gl, true);
}

void MappingManager::startDestinationMap(ConnectionButton* source, const juce::MouseEvent& e) {
    if (!hasFreeConnection()) return;

    current_source_ = source;
    dragging_ = true;

    mouse_drag_position_ = getLocalPoint(source, e.getPosition());
    positionDragIcon();

    juce::Rectangle<int> global_bounds = getLocalArea(current_source_, current_source_->getLocalBounds());
    juce::Point<int> global_start = global_bounds.getCentre();
    mouse_drag_start_ = global_start;
    destinations_->setVisible(true);
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
  std::vector<electrosynth::Connection*> connections = parent->getSynth()->getSourceConnections(source_name);
  for (electrosynth::Connection* connection : connections)
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
                                      ? SynthSlider::kNumSlots : 1;
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

void MappingManager::setDestinationQuadBounds(ConnectionDestination* destination) {

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

bool MappingManager::isPointInsideDestinationDropArea(SynthSlider* slider, juce::Point<int> manager_position) const {
    if (slider == nullptr) return false;

    const auto slider_top_left = getLocalPoint (slider, juce::Point<int>());
    const auto slider_bounds = (slider->getLocalBounds() + slider_top_left).toFloat();
    const float radius = 0.65f * std::min(slider_bounds.getWidth(), slider_bounds.getHeight());

    // if the mouse is outside the radius, return
    const auto center = slider_bounds.getCentre();
    if (manager_position.toFloat().getDistanceFrom(center) < radius) return true;

    return false;
}

int MappingManager::findSlotForNewConnection(SynthSlider* slider) const {
    if (slider == nullptr) return -1;

    const std::string destination = slider->getComponentID().toStdString();
    const auto& targets = slider->getExtraModulationTargets();

    if (temporarily_set_synth_slider_ == slider && temporarily_set_slot_ >= 0) return temporarily_set_slot_;

    for (int slot = 0; slot < SynthSlider::kNumSlots; slot++)
    {
        auto* target = targets[slot];
        if (target == nullptr || !target->isShowing()) continue;
        if (!isSlotOccupied (destination, slot)) return slot;
    }

    return -1;
}

bool MappingManager::isSlotOccupied(const std::string& destination, int destination_slot) const {
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

void MappingManager::updateSlotVisuals() {
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr) return;

    std::vector<electrosynth::SlotComponent*> active_slots;

    auto get_display_label = [this](const std::string& source_name) {
        auto button = modulation_buttons_.find(source_name);
        if (button != modulation_buttons_.end() && button->second != nullptr
                && button->second->getDisplayLabel().isNotEmpty())
            return button->second->getDisplayLabel();

        return getConnectionSourceLabel(source_name);
    };

	auto& bank = parent->getSynth()->getModulationBank();
	for (int index = 0; index < electrosynth::kMaxConnections; ++index) {
		auto* connection = bank.atIndex(index);
        if (connection == nullptr || connection->destination_name.empty()
                || !juce::isPositiveAndBelow(connection->destination_slot, SynthSlider::kNumSlots))
            continue;

        auto slider = slider_model_lookup_.find(connection->destination_name);
        if (slider == slider_model_lookup_.end() || slider->second == nullptr) continue;

	    auto* target = slider->second->getExtraModulationTarget(connection->destination_slot);
	    if (auto* slot = dynamic_cast<electrosynth::SlotComponent*>(target)) {
	        if (auto* slots = dynamic_cast<ConnectionSlots*>(slot->getParentComponent())) {
	            slots->addListener(this);
        }
            active_slots.push_back(slot);

            const auto source_button = modulation_buttons_.find(connection->source_name);
            const auto source_colour = source_button != modulation_buttons_.end()
                    && source_button->second != nullptr
                ? source_button->second->getSourceColor()
                : getConnectionSourceColor(connection->source_name);

	        ConnectionSlotData data {
	            .connectionId = juce::String(connection->uuid),
                .peer = {}, // resolve from registered modulation endpoint later
                .label = get_display_label(connection->source_name),
                .colour = source_colour,
                .hasAmount = true,
                .hasBipolar = true,
	            .hasStereo = true,
                .amount = connection->getCurrentBaseValue(),
                .bipolar = connection->isBipolar(),
                .bypass = connection->isBypass(),
                .stereo = connection->isStereo()
            };
	        auto aux = aux_connections_to_from_.find(connection->index_in_all_mods); // iterator of all aux connections
            if (aux != aux_connections_to_from_.end()) {
                auto* aux_connection = bank.atIndex(aux->second);
                if (aux_connection != nullptr && !aux_connection->source_name.empty()) {
                    auto button = modulation_buttons_.find(aux_connection->source_name);
                    const auto colour = button != modulation_buttons_.end() && button->second != nullptr ?
                    button->second->getSourceColor() : getConnectionSourceColor(aux_connection->source_name);

                    data.auxiliary = ConnectionSlotData::Auxiliary {
                        .connectionId = juce::String(aux_connection->uuid),
                        .peer = {},
                        .label = get_display_label(aux_connection->source_name),
                        .colour = colour
                    };
                }
            }
	        slot->setConnection(std::move(data));

	    }
	}

    for (const auto& [name, slider] : slider_model_lookup_) {
        if (slider == nullptr) continue;

        for (auto* target : slider->getExtraModulationTargets()) {
            auto* slot = dynamic_cast<electrosynth::SlotComponent*>(target);
            if (slot == nullptr)
                continue;

            if (std::find(active_slots.begin(), active_slots.end(), slot) == active_slots.end()) {
                slot->clearConnection();
            }
        }
    }

	  // Parameter views inside sound/effect modules are rendered into cached
  // background images. Rebuild the full background after all slot states have
  // been updated so their source-colored icons are included in those caches.
    if (auto* full = parent->getGui())
        full->redoBackground();
}

// creates an auxiliary modulation connection to an existing modulation connection
void MappingManager::makeAuxilaryConnection(ModulationAmountKnob* hover_slider) {
    if (hover_slider->isCurrentModulator() || hover_slider->hasAux() || current_modulator_ == nullptr)
        return;

    std::string name = hover_slider->getOriginalName().toStdString();
    std::string source_name = current_modulator_->getComponentID().toStdString();
    electrosynth::Connection* connection = getConnection(source_name, name);
    if (connection == nullptr) {
        float value = hover_slider->getValue() * 0.5f;
        hover_slider->setValue(0.0f, sendNotificationSync);
        temporarily_set_hover_slider_ = hover_slider;

        if (!connectMapping(source_name, name)) return;

        setConnectionValues(source_name, name, value, false, false, false);
        connection = getConnection(source_name, name);
        if (connection == nullptr) return;

        int new_index = connection->index_in_all_mods;
        addAuxConnection(new_index, hover_slider->index());
        setConnectionSliderValues(new_index, value);
    }
}

void MappingManager::draggedToComponent(juce::Component* component, bool bipolar) {
    if (component == nullptr || current_modulator_ == nullptr)
        return;

    std::string destination_name = component->getComponentID().toStdString();
    auto destination_iter = destination_lookup_.find(destination_name);
    if (destination_iter == destination_lookup_.end() || destination_iter->second == nullptr)
        return;

    ConnectionDestination* destination = destination_iter->second;
    SynthSlider* slider = destination->getDestinationSlider();
    if (slider == nullptr)
        return;

    if (!isPointInsideDestinationDropArea(slider, mouse_drag_position_)) {
        if (temporarily_set_destination_ == destination)
            clearTemporaryConnection();
        return;
    }
    auto const source_name = current_modulator_->getComponentID().toStdString();

    if (getConnection(source_name, destination_name) != nullptr)
        return; // already connected, no preview creation

    const int destination_slot = findSlotForNewConnection(slider);
    if (isSlotOccupied(destination_name, destination_slot)) return; // if slot is taken, return

    if (destination_slot < 0) return;
    if (temporarily_set_destination_ == destination && temporarily_set_slot_ != destination_slot)
        clearTemporaryConnection();



    float percent = slider->valueToProportionOfLength(slider->getValue());
    float modulation_amount = 1.0f - percent;
    if (bipolar) modulation_amount = std::min(modulation_amount, percent) * 2.0f;
    modulation_amount = std::max(modulation_amount, kDefaultModulationRatio);

    if (!connectMapping(source_name, destination_name, destination_slot)) return;

    temporarily_set_destination_ = destination;
    temporarily_set_synth_slider_ = slider_model_lookup_[destination_name];
    temporarily_set_slot_ = destination_slot;
    updateSlotVisuals();
    setConnectionValues(source_name, destination_name, modulation_amount, bipolar, false, false, destination_slot);
    destination->setActive(true);
    setDestinationQuadBounds(destination);

    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    std::vector<electrosynth::Connection*> connections = parent->getSynth()->getDestinationConnections(destination_name);

    for (electrosynth::Connection* connection : connections) {
      if (connection->source_name == source_name
          && connection->destination_name == destination_name
          && connection->destination_slot == destination_slot) {
        int index = connection->index_in_all_mods;
        showConnectionAmountOverlay(modulation_icon_[index].get());
      }
    }

    setVisibleMeterBounds();
    makeConnectionsVisible(slider, true);
    DBG("modconnecte4d");
}

void MappingManager::setTemporaryConnectionBipolar(juce::Component* component, bool bipolar) {
  if (current_modulator_ == nullptr || component != temporarily_set_destination_ || component == nullptr)
    return;

  std::string source_name = current_modulator_->getComponentID().toStdString();
  std::string name = component->getComponentID().toStdString();
  ConnectionDestination* destination = destination_lookup_[name];
  SynthSlider* slider = destination->getDestinationSlider();

  float percent = slider->valueToProportionOfLength(slider->getValue());
  float modulation_amount = 1.0f - percent;
  if (bipolar)
    modulation_amount = std::min(modulation_amount, percent) * 2.0f;
  modulation_amount = std::max(modulation_amount, kDefaultModulationRatio);

  int index = getModulationIndex(source_name, name, temporarily_set_slot_);
  setConnectionValues(source_name, name, modulation_amount, bipolar, false, false, temporarily_set_slot_);
  temporarily_set_bipolar_ = bipolar;
  if (juce::isPositiveAndBelow(index, electrosynth::kMaxConnections))
    showConnectionAmountOverlay(modulation_icon_[index].get());
}

void MappingManager::clearTemporaryConnection() {
  if (temporarily_set_destination_ && current_modulator_) {
    auto* destination = temporarily_set_destination_;
    destination->setActive(false);
    std::string source_name = current_modulator_->getComponentID().toStdString();
    removeMapping(source_name, temporarily_set_synth_slider_->getComponentID().toStdString(),
                     temporarily_set_slot_);
    setDestinationQuadBounds(destination);
    temporarily_set_destination_ = nullptr;
    temporarily_set_synth_slider_ = nullptr;
    temporarily_set_slot_ = -1;
    updateSlotVisuals();

    hideConnectionAmountOverlay();
  }
}

void MappingManager::clearTemporaryHoverConnection() {
  if (temporarily_set_hover_slider_ && current_modulator_) {
    std::string name = temporarily_set_hover_slider_->getOriginalName().toStdString();

    std::string source_name = current_modulator_->getComponentID().toStdString();
    removeMapping(source_name, temporarily_set_hover_slider_->getOriginalName().toStdString());
    temporarily_set_hover_slider_ = nullptr;
  }
}

void MappingManager::mappingDragged(const juce::MouseEvent& e) {
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
  for (int i = 0; i < electrosynth::kMaxConnections; ++i) {
    if (modulation_icon_[i].get() == component)
      hover_knob = modulation_icon_[i].get();
  }

  if (hover_knob && hover_knob->isCurrentModulator())
    return;

  bool bipolar = e.mods.isAnyModifierKeyDown();
  if (temporarily_set_destination_ && temporarily_set_destination_ != component)
    clearTemporaryConnection();
  if (temporarily_set_hover_slider_ && temporarily_set_hover_slider_ != component)
    clearTemporaryHoverConnection();

  else if (temporarily_set_synth_slider_ && temporarily_set_bipolar_ != bipolar)
    setTemporaryConnectionBipolar(component, bipolar);

  if (hover_knob)
    makeAuxilaryConnection(hover_knob);
  else
    draggedToComponent(component, bipolar);
}

void MappingManager::connectionWheelMoved(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
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

void MappingManager::endDestinationMap() {
  temporarily_set_destination_ = nullptr;
  temporarily_set_synth_slider_ = nullptr;
  temporarily_set_hover_slider_ = nullptr;
  temporarily_set_slot_ = -1;
  dragging_ = false;

  setConnectionAmounts();
  current_source_ = nullptr;
  for (auto& rotary_destination_group : rotary_destinations_)
    rotary_destination_group.second->setAlpha(0.0f);

  for (auto& linear_destination_group : linear_destinations_)
    linear_destination_group.second->setAlpha(0.0f);

  destinations_->setVisible(false);
  drag_quad_.setThickness(0.0f, true);
  drag_icon_.setVisible(false);
  drag_icon_.setActive(false);
  hideConnectionAmountOverlay();
}

void MappingManager::mappingLostFocus(ConnectionButton* source) {
  clearConnectionSource();
}

void MappingManager::clearConnectionSource() {
  if (current_modulator_) {
    for (auto& selected_slider : modulation_icon_)
      selected_slider->makeVisible(false);
  }
  current_modulator_ = nullptr;
  setConnectionAmounts();
}

void MappingManager::disconnectConnection(ModulationAmountKnob* modulation_knob) {

  electrosynth::Connection* connection = getConnectionForSlider(modulation_knob);
  while (connection && !connection->source_name.empty() && !connection->destination_name.empty()) {
    removeMapping(connection->source_name, connection->destination_name, connection->destination_slot);
    connection = getConnectionForSlider(modulation_knob);
  }

  setConnectionAmounts();
}

void MappingManager::setConnectionSettings(ModulationAmountKnob* modulation_knob) {
  electrosynth::Connection* connection = getConnectionForSlider(modulation_knob);
  float value = modulation_knob->getValue();
  bool bipolar = modulation_knob->isBipolar();
  bool stereo = modulation_knob->isStereo();
  bool bypass = modulation_knob->isBypass();

  int index = modulation_knob->index();
  modulation_icon_[index]->setBipolar(bipolar);
  modulation_icon_[index]->setStereo(stereo);
  modulation_icon_[index]->setBypass(bypass);

  setConnectionValues(connection->source_name, connection->destination_name, value, bipolar, stereo, bypass,
                      connection->destination_slot);
}

void MappingManager::setConnectionBypass(ModulationAmountKnob* modulation_knob, bool bypass) {
  setConnectionSettings(modulation_knob);
}

void MappingManager::setConnectionBipolar(ModulationAmountKnob* modulation_knob, bool bipolar) {
  setConnectionSettings(modulation_knob);
}

void MappingManager::setConnectionStereo(ModulationAmountKnob* modulation_knob, bool stereo) {
  setConnectionSettings(modulation_knob);
}

void MappingManager::initOpenGlComponents(OpenGlWrapper& open_gl) {
    drag_quad_.init(open_gl);
    drag_icon_.init(open_gl);
    mapping_mode_dim_quad_.init(open_gl);
    expansion_box_->init(open_gl);

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

void MappingManager::drawDestinations(OpenGlWrapper& open_gl) {
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

void MappingManager::drawCurrentSource(OpenGlWrapper& open_gl) {
  juce::Component* component = current_modulator_;
  if (component) {
    current_quad_.setTargetComponent(component);
    if (auto* mod_button = dynamic_cast<ConnectionButton*>(component))
      current_quad_.setColor(mod_button->getSourceColor());
    current_quad_.setAlpha(1.0f);
  }
  else
    current_quad_.setAlpha(0.0f);

  current_quad_.setThickness(dragging_ ? 3.0f : 1.0f);
  current_quad_.render(open_gl, true);
}

void MappingManager::positionDragIcon() {
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

void MappingManager::drawDraggingSource(OpenGlWrapper& open_gl) {
    if (endpoint_drag_source_.has_value() && endpoint_drag_source_component_ != nullptr) {
        drag_icon_.setActive(true);
        drag_icon_.render(open_gl, true);
        return;
    }

    if (current_source_ == nullptr || temporarily_set_destination_ || temporarily_set_hover_slider_)
        return;

    drag_icon_.setActive(true);
    drag_icon_.render(open_gl, true);
}

void MappingManager::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
    if (!animate)
        return;

    ScopedLock lock(open_gl_critical_section_);

    drawMappingMode(open_gl);
    SynthSection::renderOpenGlComponents(open_gl, animate); // render existing child/open-gl components
    OpenGlComponent::setViewPort(this, open_gl);


    for (auto& callout_button : callout_buttons_) {
        if (callout_button.second->isVisible() && !callout_button.second->isInit())
            callout_button.second->renderSliderQuads(open_gl, animate);
    }


    editing_rotary_amount_quad_.render(open_gl, animate);
    editing_linear_amount_quad_.render(open_gl, animate);


    drawDestinations(open_gl);
    drawEndpointDestinations(open_gl); // valid destination arrows

    drawCurrentSource(open_gl);

    drawDraggingSource(open_gl); // draw active drag icon
}

void MappingManager::renderMeters(OpenGlWrapper& open_gl, bool animate) {
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



void MappingManager::destroyOpenGlComponents(juce::OpenGLContext& open_gl) {
  SynthSection::destroyOpenGlComponents(open_gl);

    drag_quad_.destroy(open_gl);
    drag_icon_.destroy(open_gl);
    expansion_box_->destroy(open_gl);
    mapping_mode_dim_quad_.destroy(open_gl);
    current_quad_.destroy(open_gl);


    for (auto& rotary_destination_group : rotary_destinations_)
        rotary_destination_group.second->destroy(open_gl);

    for (auto& linear_destination_group : linear_destinations_)
        linear_destination_group.second->destroy(open_gl);

    for (auto& rotary_meter_group : rotary_meters_)
        rotary_meter_group.second->destroy(open_gl);

    for (auto& linear_meter_group : linear_meters_)
        linear_meter_group.second->destroy(open_gl);
}

void MappingManager::showConnectionAmountOverlay(ModulationAmountKnob* slider) {
  electrosynth::Connection* connection = getConnection(slider->index());
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

void MappingManager::hideConnectionAmountOverlay() {
  if (changing_hover_)
    return;

  editing_rotary_amount_quad_.setAlpha(0.0f);
  editing_linear_amount_quad_.setAlpha(0.0f);
}

void MappingManager::hoverStarted(SynthSlider* slider) {
  if (changing_hover_)
    return;

  if (!enteringHoverValue())
    makeConnectionsVisible(slider, true);

  ModulationAmountKnob* amount_knob = dynamic_cast<ModulationAmountKnob*>(slider);
  if (amount_knob)
  {
      DBG(amount_knob->getName() + juce::String((uint64)(void*)amount_knob));
      showConnectionAmountOverlay (amount_knob);
  }
  else
    hideConnectionAmountOverlay();
}

void MappingManager::hoverEnded(SynthSlider* slider) {
  hideConnectionAmountOverlay();
  //cant make the modulation go away on destinatino hover end becuase then you can't ever get to the modualtion
  //could iomplement with some sort of short timer ?
//  if (changing_hover_modulation_)
//      return;
//  makeModulationsVisible(slider, false);
}

void MappingManager::menuFinished(SynthSlider* slider) {
  if (current_modulator_ && current_modulator_->isVisible())
    current_modulator_->grabKeyboardFocus();
}

void MappingManager::modulationsChanged(const std::string& destination) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();

  hideUnusedHoverModulations();
  updateSlotVisuals();
  SynthSlider* slider = slider_model_lookup_[destination];
  if (current_modulator_)
    makeCurrentConnectionAmountsVisible();
  else if (slider)
    makeConnectionsVisible(slider, slider->isShowing());

  if (parent == nullptr)
    return;

  if (!meter_lookup_.contains (destination))
    return;

  int num_modulations = parent->getSynth()->getNumModulations(destination);
  meter_lookup_[destination]->setModulated(num_modulations);
  meter_lookup_[destination]->setVisible(num_modulations);
}

int MappingManager::getModulationIndex(std::string source, std::string destination, int destination_slot) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  std::vector<electrosynth::Connection*> connections = parent->getSynth()->getDestinationConnections(destination);

  for (electrosynth::Connection* connection : connections) {
    if (connection->source_name == source
        && (destination_slot < 0 || connection->destination_slot == destination_slot))
      return connection->index_in_all_mods;
  }

  return -1;
}

int MappingManager::getIndexForModulationSlider(juce::Slider* slider) {
  ModulationAmountKnob* amount_knob = dynamic_cast<ModulationAmountKnob*>(slider);
  if (amount_knob)
    return amount_knob->index();
  return -1;
}

electrosynth::Connection* MappingManager::getConnectionForSlider(juce::Slider* slider) {
  int index = getIndexForModulationSlider(slider);
  if (index < 0)
    return nullptr;

  while (aux_connections_to_from_.count(index))
    index = aux_connections_to_from_[index];

  return getConnection(index);
}

electrosynth::Connection* MappingManager::getConnection(int index) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return nullptr;

  return parent->getSynth()->getModulationBank().atIndex(index);
}

electrosynth::Connection* MappingManager::getConnection(const std::string& source, const std::string& dest,
                                                                     int destination_slot) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return nullptr;

  std::vector<electrosynth::Connection*> connections = parent->getSynth()->getSourceConnections(source);
  for (electrosynth::Connection* connection : connections) {
    if (connection->destination_name == dest
        && (destination_slot < 0 || connection->destination_slot == destination_slot))
      return connection;
  }

  return nullptr;
}

void MappingManager::mouseDown(SynthSlider* slider) {
    // ignore clicks on modulation amount knobs (the sliders under the synth knobs)
  for (auto& amount_knob : modulation_icon_) {
    if (slider == amount_knob.get())
      return;
  }

  if (expansion_box_->isVisible())
    return;

    // if there is modulation connected to this slider, select that modulation source
  electrosynth::Connection* connection = getConnectionForSlider(slider);
  if (connection && !connection->source_name.empty() && !connection->destination_name.empty())
    connectionSelected(modulation_buttons_[connection->source_name]);
  else {
    clearConnectionSource();
    hideConnectionAmountOverlay();
    makeConnectionsVisible(slider, true);
  }
}

void MappingManager::mouseUp(SynthSlider* slider) {
  if (current_modulator_ && current_modulator_->isVisible())
    current_modulator_->grabKeyboardFocus();

}

void MappingManager::doubleClick(SynthSlider* slider) {
  changing_hover_ = false;
  electrosynth::Connection* connection = getConnectionForSlider(slider);
//  if (connection)
//    removeModulation(connection->source_name, connection->destination_name);
//  setModulationAmounts();
//
//  if (current_modulator_ && current_modulator_->isVisible())
//    current_modulator_->grabKeyboardFocus();
}



void MappingManager::sliderValueChanged(juce::Slider* slider) {
  ModulationAmountKnob* amount_knob = dynamic_cast<ModulationAmountKnob*>(slider);
  if (amount_knob == nullptr)
    return;

  float value = slider->getValue();
  int index = getIndexForModulationSlider(slider);
  float scale = getAuxMultiplier(index);
  float scaled_value = value * scale;
  while (aux_connections_to_from_.count(index))
    index = aux_connections_to_from_[index];

	  electrosynth::Connection* connection = getConnection(index);
  if (connection == nullptr)
    return;

  bool bipolar = connection->isBipolar();
	  bool stereo = connection->isStereo();
	  bool bypass = connection->isBypass();
  connection->setScalingValue(value);
//
  setConnectionValues(connection->source_name, connection->destination_name, scaled_value, bipolar, stereo, bypass,
                      connection->destination_slot);
	  updateSlotVisuals();
  showConnectionAmountOverlay(amount_knob);
//
  SynthSection::sliderValueChanged(modulation_icon_[index].get());
}

void MappingManager::buttonClicked(juce::Button* button) {
    for (auto& callout_button : callout_buttons_) {
        if (button == callout_button.second.get()) {
            bool new_button = button != current_expanded_;
            hideConnectionAmountCallout();
            if (new_button) showConnectionAmountCallout(callout_button.first);
            return;
        }
}

  SynthSection::buttonClicked(button);
}

bool MappingManager::connectMapping(
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

void MappingManager::removeMapping(std::string source, std::string destination, int destination_slot) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || source.empty() || destination.empty())
    return;

  electrosynth::Connection* connection = getConnection(source, destination, destination_slot);
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
  updateSlotVisuals();
  modulationsChanged(destination);
  modifying_ = false;
}

void MappingManager::setConnectionSliderValue(int index, float value) {
  modulation_icon_[index]->setValue(value, dontSendNotification);
  modulation_icon_[index]->redoImage();
}

void MappingManager::setConnectionSliderBipolar(int index, bool bipolar) {
  modulation_icon_[index]->setBipolar(bipolar);
}

void MappingManager::setConnectionSliderValues(int index, float value) {
  setConnectionSliderValue(index, value);
  float from_value = value;
  int from_index = index;
  while (aux_connections_from_to_.count(from_index)) {
    from_index = aux_connections_from_to_[from_index];
    from_value *= 2.0f;
    setConnectionSliderValue(from_index, from_value);
  }

  float to_value = value;
  int to_index = index;
  while (aux_connections_to_from_.count(to_index)) {
    to_index = aux_connections_to_from_[to_index];
    to_value *= 0.5f;
    setConnectionSliderValue(to_index, to_value);
  }

  setConnectionSliderScale(index);
}

void MappingManager::setConnectionSliderScale(int index) {
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

void MappingManager::setConnectionValues(std::string source, std::string destination,
                                            float amount, bool bipolar, bool stereo, bool bypass,
                                            int destination_slot) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || source.empty() || destination.empty())
    return;

  modifying_ = true;
//  parent->setModulationValues(source, destination, amount, bipolar, stereo, bypass);
  int index = getModulationIndex(source, destination, destination_slot);
//  parent->notifyModulationValueChanged(index);
  if (juce::isPositiveAndBelow(index, electrosynth::kMaxConnections)) {
    electrosynth::Connection* connection = getConnection(index);
    if (connection != nullptr) {
      connection->setBipolar(bipolar);
      connection->setStereo(stereo);
      connection->setBypass(bypass);
      connection->setScalingValue(amount);
    }
    setConnectionSliderValues(index, amount);
    setConnectionSliderBipolar(index, bipolar);
  }
  updateSlotVisuals();
//
  modifying_ = false;
}

void MappingManager::initAuxConnections() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  for (int i = 0; i < electrosynth::kMaxConnections; ++i) {
    modulation_icon_[i]->removeAux();
  }

  aux_connections_from_to_.clear();
  aux_connections_to_from_.clear();
//
  electrosynth::ConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < electrosynth::kMaxConnections; ++i) {
    electrosynth::Connection* connection = bank.atIndex(i);
    int index = connection->index_in_all_mods;

    if (modulation_amount_lookup_.count(connection->destination_name)) {
      int modulation_index = modulation_amount_lookup_[connection->destination_name]->index();
      addAuxConnection(index, modulation_index);
    }
  }
}

void MappingManager::reset() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || modifying_)
    return;

  for (auto& meter : meter_lookup_) {
    int num_modulations = parent->getSynth()->getNumModulations(meter.first);
    meter.second->setModulated(num_modulations);
    meter.second->setVisible(num_modulations);
  }

  setConnectionAmounts();
  initAuxConnections();
  updateSlotVisuals();
}

void MappingManager::hideUnusedHoverModulations() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || changing_hover_)
    return;

  electrosynth::ConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < electrosynth::kMaxConnections; ++i) {
    electrosynth::Connection* connection = bank.atIndex(i);
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

float MappingManager::getAuxMultiplier(int index) {
  float mult = 1.0f;
  while (aux_connections_to_from_.count(index)) {
    index = aux_connections_to_from_[index];
    mult *= 0.5f;
  }

  return mult;
}

void MappingManager::addAuxConnection(int from_index, int to_index) {
  if (from_index == to_index)
    return;

  aux_connections_to_from_[to_index] = from_index;
  aux_connections_from_to_[from_index] = to_index;
  std::string aux_name = "modulation_" + std::to_string(from_index + 1) + "_amount";
  modulation_icon_[to_index]->setAux(aux_name);
  updateSlotVisuals();

}

void MappingManager::removeAuxSourceConnection(int from_index) {
  if (aux_connections_from_to_.count(from_index) == 0)
    return;

  int to_index = aux_connections_from_to_[from_index];
  modulation_icon_[to_index]->removeAux();
  aux_connections_from_to_.erase(from_index);
  aux_connections_to_from_.erase(to_index);
  updateSlotVisuals();
}

void MappingManager::removeAuxDestinationConnection(int to_index) {
  if (aux_connections_to_from_.count(to_index) == 0)
    return;

  modulation_icon_[to_index]->removeAux();
  aux_connections_from_to_.erase(aux_connections_to_from_[to_index]);
  aux_connections_to_from_.erase(to_index);
  updateSlotVisuals();
}

void MappingManager::makeCurrentConnectionAmountsVisible() {
    for (auto& selected_slider : modulation_icon_)
        selected_slider->makeVisible(false);

    positionConnectionAmountSliders();
}

ModulationAmountKnob* MappingManager::getConnectionAmountControl(const electrosynth::Connection* connection) const {
    if (connection == nullptr
        || !juce::isPositiveAndBelow(connection->index_in_all_mods, electrosynth::kMaxConnections))
        return nullptr;

    return modulation_icon_[connection->index_in_all_mods].get();
}

void MappingManager::syncConnectionAmountControl(electrosynth::Connection* connection,
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

bool MappingManager::placeConnectionAmountInSlot(SynthSlider* destination,
    const electrosynth::Connection* connection, ModulationAmountKnob* amount_knob) {
    if (destination == nullptr || connection == nullptr || amount_knob == nullptr
      || !juce::isPositiveAndBelow(connection->destination_slot, SynthSlider::kNumSlots))
        return false;

    return dynamic_cast<electrosynth::SlotComponent*>(destination->getExtraModulationTarget(
            connection->destination_slot)) != nullptr;
}

void MappingManager::makeConnectionsVisible(SynthSlider* destination, bool visible) {

    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (destination == nullptr || parent == nullptr || changing_hover_)
        return;

    std::string name = destination->getComponentID().toStdString();
    auto slider_iter = slider_model_lookup_.find(name);
    if (slider_iter == slider_model_lookup_.end() || slider_iter->second != destination)
        return;


    std::vector<electrosynth::Connection*> connections = parent->getSynth()->getDestinationConnections(name);
    int num_amount_controls = 0;

    for (electrosynth::Connection* connection : connections) {
        auto* amount_knob = getConnectionAmountControl(connection);
        if (amount_knob == nullptr) continue;
        syncConnectionAmountControl(connection, amount_knob);
        if (!placeConnectionAmountInSlot(destination, connection, amount_knob))
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

  for (electrosynth::Connection* connection : connections) {
    auto* amount_knob = getConnectionAmountControl(connection);
    if (amount_knob == nullptr)
      continue;

    const bool placed_in_slot = placeConnectionAmountInSlot(destination, connection, amount_knob);
    if (placed_in_slot) {
        amount_knob->makeVisible(false);
        amount_knob->setInterceptsMouseClicks(false, false);
        continue;
    }

    amount_knob->setInterceptsMouseClicks(true, true);
    amount_knob->setPopupPlacement(placement);
    amount_knob->setBounds(x, y, amount_control_width, amount_control_width);
    amount_knob->setAlwaysOnTop(false);
    amount_knob->getQuadComponent()->setAlwaysOnTop(false);
    amount_knob->getImageComponent()->setAlwaysOnTop(false);
    amount_knob->getQuadComponent()->setVisible(true);
    amount_knob->getImageComponent()->setVisible(true);
    amount_knob->makeVisible(visible && allVisible(destination));
    amount_knob->setAlpha(1.0f, true);
    amount_knob->redoImage();

    x += delta_x;
    y += delta_y;
  }
}

void MappingManager::positionConnectionAmountSliders() {
  for (const auto& [name, slider] : slider_model_lookup_)
    makeConnectionsVisible(slider, slider != nullptr && slider->isShowing());
}

void MappingManager::showConnectionAmountCallout(const std::string& source) {
  static constexpr int kSliderWidth = 30;
  static constexpr int kPadding = 5;

  ConnectionButton* modulation_button = modulation_buttons_[source];
  current_expanded_ = callout_buttons_[source].get();
  std::vector<ModulationAmountKnob*> amount_controls = current_expanded_->getSliders();

  int num_sliders = static_cast<int>(amount_controls.size());
  int columns = current_expanded_->getNumColumns(num_sliders);
  int rows = (num_sliders + columns - 1) / columns;
  int width = kSliderWidth * columns + 2 * kPadding;
  int height = kSliderWidth * rows + 2 * kPadding;
  juce::Rectangle<int> top_level_modulation_bounds = getLocalArea(modulation_button, modulation_button->getLocalBounds());
  int start_x = top_level_modulation_bounds.getX() + (modulation_button->getWidth() - width) / 2;
  start_x = std::min(getWidth() - width, std::max(0, start_x));
  int start_y = top_level_modulation_bounds.getBottom();
  start_y = std::min(getHeight() - height, start_y);

  expansion_box_->setVisible(true);
  expansion_box_->setAmountControls(amount_controls);
  expansion_box_->setBounds(start_x, start_y, width, height);
  expansion_box_->setRounding(findValue(Skin::kBodyRounding));
  expansion_box_->grabKeyboardFocus();

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

void MappingManager::hideConnectionAmountCallout() {
  if (current_expanded_ == nullptr)
    return;

  std::vector<ModulationAmountKnob*> amount_controls = current_expanded_->getSliders();
  for (ModulationAmountKnob* slider : amount_controls) {
    slider->setVisible(false);
    slider->getQuadComponent()->setAlwaysOnTop(false);
  }

  expansion_box_->setVisible(false);
  current_expanded_ = nullptr;
}


bool MappingManager::enteringHoverValue() {
  for (int i = 0; i < electrosynth::kMaxConnections; ++i) {
    if (modulation_icon_[i] && modulation_icon_[i]->enteringValue())
      return true;
  }
  return false;
}

void MappingManager::setConnectionAmounts() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || modifying_)
    return;

  electrosynth::ConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < electrosynth::kMaxConnections; ++i) {
    electrosynth::Connection* connection = bank.atIndex(i);
    if (aux_connections_to_from_.count(i) == 0)
      setConnectionSliderValues(i, connection->getCurrentBaseValue());

    bool bipolar = connection->isBipolar();
    bool stereo = connection->isStereo();
    bool bypass = connection->isBypass();

    modulation_icon_[i]->setBipolar(bipolar);
    modulation_icon_[i]->setStereo(stereo);
    modulation_icon_[i]->setBypass(bypass);
  }
}

void MappingManager::setVisibleMeterBounds() {
  for (auto& meter : meter_lookup_) {
    SynthSlider* slider = slider_model_lookup_[meter.first];
    if (slider && slider->isShowing()) {
      juce::Rectangle<int> local_bounds = getLocalArea(slider, slider->getModulationMeterBounds());
      meter.second->setBounds(local_bounds);
    }
  }
}
