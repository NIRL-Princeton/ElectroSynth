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

MappingManager::MappingManager() :
        SynthSection("modulation_manager"), drag_quad_(Shaders::kRingFragment), drag_icon_("modulation_drag_icon"),
        current_quad_(Shaders::kRoundedRectangleBorderFragment),
        mapping_mode_dim_quad_(Shaders::kColorFragment, "modulation_mapping_mode_dim"),
        editing_rotary_amount_quad_(Shaders::kRotaryModulationFragment),
        editing_linear_amount_quad_(Shaders::kLinearModulationFragment), modifying_(false), dragging_(false),
        changing_hover_(false),component_update_pending_(false), current_modulator_(nullptr) {

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

    setSkinOverride(Skin::kModulationDragDrop);

    current_source_ = nullptr;
    temporarily_set_destination_ = nullptr;
    temporarily_set_synth_slider_ = nullptr;
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
bool MappingManager::endpointsAreCompatible(const electrosynth::EndpointAddress& source, const electrosynth::EndpointAddress& destination)
{
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

    auto* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr) return false;

    const auto existingConnections = parent->getConnectionsForEndpoint(destination);

    const auto duplicate = std::find_if(existingConnections.begin(), existingConnections.end(),
       [&source, &destination](const auto& record) {
           return record.source.matches(source) && record.destination.matches(destination);
       });
    if (duplicate != existingConnections.end()) return true;

    const int capacity = destination_component->getEndpoint().capabilities.maxIncomingConnections;
    if (capacity <= 0 || static_cast<int>(existingConnections.size()) >= capacity) {
        return false;
    }

    electrosynth::ConnectionRecord connection {
        .id = electrosynth::createConnectionRecordId(),
        .type = source.type,
        .source = source,
        .destination = destination
    };

    if (!parent->connect(connection))
        return false;

    updateConnectionSlots();
    return true;
}

void MappingManager::updateConnectionSlots() {

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

    auto* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr) return;

    for (auto& [key, registered_endpoint] : mapping_endpoints_) {
        auto* port = registered_endpoint.component.getComponent();
        if (port == nullptr) continue;

        const auto& address = port->getEndpoint().address;
        if (address.type != electrosynth::ConnectionType::Audio) continue;

        std::vector<ConnectionSlotData> slots_for_port;
        const auto endpointConnections = parent->getConnectionsForEndpoint(address);
        for (const auto& connection : endpointConnections) {
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

    auto* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr) return;

    const auto* stored = parent->findConnection(connectionId);

    if (stored == nullptr)
        return;

    auto updated = *stored;
    switch (result) {
        case kToggleConnectionBypass:
            updated.bypass = !updated.bypass;
            break;
        case kToggleConnectionBipolar:
            updated.bipolar = !updated.bipolar;
            break;
        case kToggleConnectionStereo:
            updated.stereo = !updated.stereo;
            break;
        default:
            return;
    }

    if (parent->updateConnection(updated)) {
        updateConnectionSlots();
        updateSlotVisuals();
    }
}

void MappingManager::removeConnectionRecord(const juce::String& connectionId) {

    auto* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr) return;

    const auto* connection = parent->findConnection(connectionId);
    if (connection == nullptr)
        return;

    const auto destination = connection->destination.endpointId.toStdString();
    const auto type = connection->type;
    if (parent->disconnectConnection(connectionId)) {
        updateConnectionSlots();
        updateSlotVisuals();
        if (type == electrosynth::ConnectionType::Modulation)
            modulationsChanged(destination);
    }
}

void MappingManager::connectionAmountChanged(const ConnectionSlotData& connection, float amount)  {

    auto* parent =findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return;

    if (const auto* stored = parent->findConnection(connection.connectionId)) {
        auto updated = *stored;
        updated.amount = amount;

        if (parent->updateConnection(updated)) {
            updateConnectionSlots();
            updateSlotVisuals();
        }
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
      int num_modulations = 0;
      if (model != nullptr && model->hasModulationEndpoint()) {
        const auto connections = parent->getConnectionsForEndpoint(model->getModulationEndpoint().address);
        num_modulations = static_cast<int>(std::count_if(connections.begin(), connections.end(), [](const auto& connection) {
          return connection.type == electrosynth::ConnectionType::Modulation
              && connection.targetConnectionId.isEmpty();
        }));
      }
      meter.second->setModulated(num_modulations);
      meter.second->setVisible(num_modulations);
    }
  }
}

void MappingManager::connectionAmountChanged(SynthSlider* slider) {
  if (slider == nullptr || current_modulator_ == nullptr || !slider->hasModulationEndpoint()
      || !current_modulator_->hasEndpoint())
    return;

  auto* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  for (const auto& connection : parent->getConnectionsForEndpoint(slider->getModulationEndpoint().address)) {
    if (!connection.source.matches(current_modulator_->getEndpoint().address)
        || connection.targetConnectionId.isNotEmpty())
      continue;

    auto updated = connection;
    updated.amount = slider->getModulationAmount();
    updated.bipolar = slider->isModulationBipolar();
    updated.stereo = slider->isModulationStereo();
    updated.bypass = slider->isModulationBypassed();
    parent->updateConnection(updated);
    updateSlotVisuals();
    current_modulator_->repaint();
    return;
  }
}

void MappingManager::connectionRemoved(SynthSlider* slider) {
  if (slider == nullptr || current_modulator_ == nullptr || !slider->hasModulationEndpoint()
      || !current_modulator_->hasEndpoint())
    return;

  auto* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  for (const auto& connection : parent->getConnectionsForEndpoint(slider->getModulationEndpoint().address)) {
    if (connection.source.matches(current_modulator_->getEndpoint().address)
        && connection.targetConnectionId.isEmpty()) {
      parent->disconnectConnection(connection.id);
      modulationsChanged(slider->getComponentID().toStdString());
      current_modulator_->repaint();
      return;
    }
  }
}

void MappingManager::connectionSelected(ConnectionButton* source) {

  current_modulator_ = source;
}

void MappingManager::connectionClicked(ConnectionButton* source) {
  juce::ignoreUnused(source);
}

bool MappingManager::hasFreeConnection() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return false;

  const auto connections = parent->getConnections();
  return std::count_if(connections.begin(), connections.end(), [](const auto& connection) {
    return connection.type == electrosynth::ConnectionType::Modulation
        && connection.targetConnectionId.isEmpty();
  }) < electrosynth::kMaxConnections;
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
        dragging_ = false;
        current_source_ = nullptr;
        current_modulator_ = nullptr;
        temporarily_set_destination_ = nullptr;
        temporarily_set_synth_slider_ = nullptr;
        temporarily_set_connection_id_.clear();
        temporarily_set_slot_ = -1;
        destinations_->setVisible(false);


        rotary_destinations_.clear();
        rotary_meters_.clear();
        linear_destinations_.clear();
        linear_meters_.clear();
        destination_lookup_.clear();
        all_destinations_.clear();
        modulation_buttons_.clear();
        meter_lookup_.clear();
        num_linear_meters.clear();
        num_rotary_meters.clear();
        modulation_buttons_ = mod_buttons;
        for (auto& modulation_button : modulation_buttons_) {
            if (!modulation_button.second->hasEndpoint()) {
                modulation_button.second->addListener(this);
            }
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
  std::set<std::string> active_destinations;
  if (parent != nullptr && source->hasEndpoint()) {
    for (const auto& connection : parent->getConnectionsForEndpoint(source->getEndpoint().address))
      active_destinations.insert(connection.destination.endpointId.toStdString());
  }

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

  const auto slider = slider_model_lookup_.find(destination);
  if (slider == slider_model_lookup_.end() || slider->second == nullptr
      || !slider->second->hasModulationEndpoint())
    return false;

  for (const auto& connection : parent->getConnectionsForEndpoint(
           slider->second->getModulationEndpoint().address)) {
    if (connection.targetConnectionId.isEmpty()
        && connection.destinationSlot == destination_slot)
      return true;
  }

  return false;
}

void MappingManager::updateSlotVisuals() {
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr) return;

    std::vector<electrosynth::SlotComponent*> active_slots;

    auto get_display_label = [this](const juce::String& source_name) {
        auto button = modulation_buttons_.find(source_name.toStdString());
        if (button != modulation_buttons_.end() && button->second != nullptr
                && button->second->getDisplayLabel().isNotEmpty())
            return button->second->getDisplayLabel();

        return getConnectionSourceLabel(source_name.toStdString());
    };

    for (const auto& [name, slider] : slider_model_lookup_) {
        if (slider == nullptr || !slider->hasModulationEndpoint())
            continue;

        for (const auto& connection : parent->getConnectionsForEndpoint(
                 slider->getModulationEndpoint().address)) {
            if (connection.type != electrosynth::ConnectionType::Modulation
                || connection.targetConnectionId.isNotEmpty()
                || !juce::isPositiveAndBelow(connection.destinationSlot, SynthSlider::kNumSlots))
                continue;

            auto* target = slider->getExtraModulationTarget(connection.destinationSlot);
            auto* slot = dynamic_cast<electrosynth::SlotComponent*>(target);
            if (slot == nullptr)
                continue;

            if (auto* slots = dynamic_cast<ConnectionSlots*>(slot->getParentComponent()))
                slots->addListener(this);
            active_slots.push_back(slot);

            const auto sourceName = connection.source.endpointId;
            const auto sourceButton = modulation_buttons_.find(sourceName.toStdString());
            const auto sourceColour = sourceButton != modulation_buttons_.end()
                    && sourceButton->second != nullptr
                ? sourceButton->second->getSourceColor()
                : getConnectionSourceColor(sourceName.toStdString());

            ConnectionSlotData data {
                .connectionId = connection.id,
                .peer = connection.source,
                .label = get_display_label(sourceName),
                .colour = sourceColour,
                .hasAmount = true,
                .hasBipolar = true,
                .hasStereo = true,
                .amount = connection.amount,
                .bipolar = connection.bipolar,
                .bypass = connection.bypass,
                .stereo = connection.stereo
            };

            const auto auxiliaryConnections = parent->getConnectionsTargetingConnection(connection.id);
            if (!auxiliaryConnections.empty()) {
                const auto& auxiliary = auxiliaryConnections.front();
                const auto auxiliaryName = auxiliary.source.endpointId;
                const auto button = modulation_buttons_.find(auxiliaryName.toStdString());
                const auto colour = button != modulation_buttons_.end() && button->second != nullptr
                    ? button->second->getSourceColor()
                    : getConnectionSourceColor(auxiliaryName.toStdString());

                data.auxiliary = ConnectionSlotData::Auxiliary {
                    .connectionId = auxiliary.id,
                    .peer = auxiliary.source,
                    .label = get_display_label(auxiliaryName),
                    .colour = colour
                };
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

void MappingManager::draggedToComponent(juce::Component* component, bool bipolar) {
    if (component == nullptr || current_modulator_ == nullptr || !current_modulator_->hasEndpoint())
        return;

    std::string destination_name = component->getComponentID().toStdString();
    auto destination_iter = destination_lookup_.find(destination_name);
    if (destination_iter == destination_lookup_.end() || destination_iter->second == nullptr)
        return;

    ConnectionDestination* destination = destination_iter->second;
    SynthSlider* slider = destination->getDestinationSlider();
    if (slider == nullptr || !slider->hasModulationEndpoint())
        return;

    if (!isPointInsideDestinationDropArea(slider, mouse_drag_position_)) {
        if (temporarily_set_destination_ == destination)
            clearTemporaryConnection();
        return;
    }
    auto* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent == nullptr)
        return;

    const auto sourceAddress = current_modulator_->getEndpoint().address;
    const auto destinationAddress = slider->getModulationEndpoint().address;
    const auto existingConnections = parent->getConnectionsForEndpoint(destinationAddress);
    const auto duplicate = std::find_if(existingConnections.begin(), existingConnections.end(),
        [&sourceAddress](const auto& connection) {
            return connection.targetConnectionId.isEmpty()
                && connection.source.matches(sourceAddress);
        });
    if (duplicate != existingConnections.end())
        return;

    const int destination_slot = findSlotForNewConnection(slider);
    if (isSlotOccupied(destination_name, destination_slot)) return; // if slot is taken, return

    if (destination_slot < 0) return;
    if (temporarily_set_destination_ == destination && temporarily_set_slot_ != destination_slot)
        clearTemporaryConnection();



    float percent = slider->valueToProportionOfLength(slider->getValue());
    float modulation_amount = 1.0f - percent;
    if (bipolar) modulation_amount = std::min(modulation_amount, percent) * 2.0f;
    modulation_amount = std::max(modulation_amount, kDefaultModulationRatio);

    electrosynth::ConnectionRecord connection {
        .id = electrosynth::createConnectionRecordId(),
        .type = electrosynth::ConnectionType::Modulation,
        .source = sourceAddress,
        .destination = destinationAddress,
        .destinationSlot = destination_slot,
        .amount = modulation_amount,
        .bipolar = bipolar
    };

    modifying_ = true;
    const bool connected = parent->connect(connection);
    modifying_ = false;
    if (!connected)
        return;

    temporarily_set_destination_ = destination;
    temporarily_set_synth_slider_ = slider_model_lookup_[destination_name];
    temporarily_set_connection_id_ = connection.id;
    temporarily_set_slot_ = destination_slot;
    temporarily_set_bipolar_ = bipolar;
    updateSlotVisuals();
    modulationsChanged(destination_name);
    destination->setActive(true);
    setDestinationQuadBounds(destination);
    showConnectionAmountOverlay(temporarily_set_connection_id_);

    setVisibleMeterBounds();
    DBG("modconnecte4d");
}

void MappingManager::setTemporaryConnectionBipolar(juce::Component* component, bool bipolar) {
  if (current_modulator_ == nullptr || component != temporarily_set_destination_ || component == nullptr)
    return;

  std::string name = component->getComponentID().toStdString();
  ConnectionDestination* destination = destination_lookup_[name];
  SynthSlider* slider = destination->getDestinationSlider();

  float percent = slider->valueToProportionOfLength(slider->getValue());
  float modulation_amount = 1.0f - percent;
  if (bipolar)
    modulation_amount = std::min(modulation_amount, percent) * 2.0f;
  modulation_amount = std::max(modulation_amount, kDefaultModulationRatio);

  auto* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  const auto* stored = parent->findConnection(temporarily_set_connection_id_);
  if (stored == nullptr)
    return;

  auto updated = *stored;
  updated.amount = modulation_amount;
  updated.bipolar = bipolar;
  if (!parent->updateConnection(updated))
    return;

  temporarily_set_bipolar_ = bipolar;
  updateSlotVisuals();
  showConnectionAmountOverlay(temporarily_set_connection_id_);
}

void MappingManager::clearTemporaryConnection() {
  if (temporarily_set_destination_ && current_modulator_) {
    auto* destination = temporarily_set_destination_;
    const auto destinationName = destination->getComponentID().toStdString();
    destination->setActive(false);
    if (auto* parent = findParentComponentOfClass<SynthGuiInterface>())
      parent->disconnectConnection(temporarily_set_connection_id_);
    setDestinationQuadBounds(destination);
    temporarily_set_destination_ = nullptr;
    temporarily_set_synth_slider_ = nullptr;
    temporarily_set_connection_id_.clear();
    temporarily_set_slot_ = -1;
    updateSlotVisuals();
    modulationsChanged(destinationName);

    hideConnectionAmountOverlay();
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

  bool bipolar = e.mods.isAnyModifierKeyDown();
  if (temporarily_set_destination_ && temporarily_set_destination_ != component)
    clearTemporaryConnection();
  else if (temporarily_set_synth_slider_ && temporarily_set_bipolar_ != bipolar)
    setTemporaryConnectionBipolar(component, bipolar);

  draggedToComponent(component, bipolar);
}

void MappingManager::connectionWheelMoved(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
  juce::ignoreUnused(e, wheel);
}

void MappingManager::endDestinationMap() {
  temporarily_set_destination_ = nullptr;
  temporarily_set_synth_slider_ = nullptr;
  temporarily_set_connection_id_.clear();
  temporarily_set_slot_ = -1;
  dragging_ = false;

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
  current_modulator_ = nullptr;
}

void MappingManager::drawDestinations(OpenGlWrapper& open_gl) const {
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

    if (current_source_ == nullptr || temporarily_set_destination_)
        return;

    drag_icon_.setActive(true);
    drag_icon_.render(open_gl, true);
}

void MappingManager::initOpenGlComponents(OpenGlWrapper& open_gl) {
    drag_quad_.init(open_gl);
    drag_icon_.init(open_gl);
    mapping_mode_dim_quad_.init(open_gl);

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

void MappingManager::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
    if (!animate)
        return;

    ScopedLock lock(open_gl_critical_section_);

    drawMappingMode(open_gl);
    SynthSection::renderOpenGlComponents(open_gl, animate); // render existing child/open-gl components
    OpenGlComponent::setViewPort(this, open_gl);

    editing_rotary_amount_quad_.render(open_gl, animate);
    editing_linear_amount_quad_.render(open_gl, animate);

    drawDestinations(open_gl);
    drawEndpointDestinations(open_gl); // valid destination arrows

    drawCurrentSource(open_gl);
    drawDraggingSource(open_gl); // draw active drag icon
}

void MappingManager::destroyOpenGlComponents(juce::OpenGLContext& open_gl) {
    SynthSection::destroyOpenGlComponents(open_gl);

    drag_quad_.destroy(open_gl);
    drag_icon_.destroy(open_gl);
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

                const auto sliderIt = slider_model_lookup_.find(meter.first);
                const auto connections = sliderIt != slider_model_lookup_.end() && sliderIt->second != nullptr
                        && sliderIt->second->hasModulationEndpoint()
                    ? parent->getConnectionsForEndpoint(sliderIt->second->getModulationEndpoint().address)
                    : std::vector<electrosynth::ConnectionRecord>{};

                for (const auto& connection : connections) {
                    if (connection.targetConnectionId.isEmpty() && !connection.bypass) {
                        display_value += connection.amount * range;
                        float amount = connection.amount;
                        if (connection.bipolar)
                            amount *= 2.0f;
                        meter.second->setStaticModulationAmount(amount, connection.bipolar);
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

void MappingManager::showConnectionAmountOverlay(const juce::String& connectionId) {
  auto* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  const auto* connection = parent->findConnection(connectionId);
  if (connection == nullptr || connection->type != electrosynth::ConnectionType::Modulation
      || !meter_lookup_.contains(connection->destination.endpointId.toStdString()))
    return;

  ModulationMeter* meter = meter_lookup_[connection->destination.endpointId.toStdString()].get();
  if (!meter->destination()->isShowing())
    return;

  if (meter->isRotary()) {
      editing_rotary_amount_quad_.setTargetComponent(meter);
      editing_rotary_amount_quad_.setAdditive(false);
      meter->setAmountQuadVertices(editing_rotary_amount_quad_);
      meter->setModulationAmountQuad(editing_rotary_amount_quad_,
                                     connection->amount, connection->bipolar);

      editing_rotary_amount_quad_.setThickness(2.0f);
      editing_rotary_amount_quad_.setAlpha(1.0f);
      editing_rotary_amount_quad_.setActive(true);
  }

  else {
      editing_linear_amount_quad_.setTargetComponent(meter);
      editing_linear_amount_quad_.setAdditive(false);
      meter->setAmountQuadVertices(editing_linear_amount_quad_);
      meter->setModulationAmountQuad(editing_linear_amount_quad_,
                                     connection->amount, connection->bipolar);

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

  updateSlotVisuals();
  SynthSlider* slider = slider_model_lookup_[destination];

  if (parent == nullptr)
    return;

  if (!meter_lookup_.contains (destination))
    return;

  int num_modulations = 0;
  if (slider != nullptr && slider->hasModulationEndpoint()) {
    const auto connections = parent->getConnectionsForEndpoint(slider->getModulationEndpoint().address);
    num_modulations = static_cast<int>(std::count_if(connections.begin(), connections.end(), [](const auto& connection) {
      return connection.type == electrosynth::ConnectionType::Modulation
          && connection.targetConnectionId.isEmpty();
    }));
  }
  meter_lookup_[destination]->setModulated(num_modulations);
  meter_lookup_[destination]->setVisible(num_modulations);
}

void MappingManager::mouseDown(SynthSlider* slider) {
  juce::ignoreUnused(slider);
}

void MappingManager::mouseUp(SynthSlider* slider) {
}

void MappingManager::doubleClick(SynthSlider* slider) {
  juce::ignoreUnused(slider);
  changing_hover_ = false;
}

void MappingManager::sliderValueChanged(juce::Slider* slider) {
  SynthSection::sliderValueChanged(slider);
}

void MappingManager::reset() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || modifying_)
    return;

  for (const auto& meter : meter_lookup_)
    modulationsChanged(meter.first);
  updateSlotVisuals();
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
