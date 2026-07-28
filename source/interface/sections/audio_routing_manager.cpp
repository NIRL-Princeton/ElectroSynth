//
// Created by Callista Chong on 7/22/26.
//

#include "audio_routing_manager.h"

#include "AudioConnection.h"
#include "audio_connection_slots.h"
#include "chowdsp_sources/chowdsp_sources.h"

namespace {
    juce::String abbreviateLabel(const juce::String& label) {
        const auto abbreviate_prefix = [&label] (const juce::String& full, const juce::String& abbreviated)
        -> std::optional<juce::String> {
            if (!label.startsWithIgnoreCase(full))
                return std::nullopt;

            const auto suffix = label.substring(full.length()).trimStart();

            return suffix.isEmpty() ? abbreviated : abbreviated + " " + suffix;
        };

        if (auto result = abbreviate_prefix("Oscillator", "osc")) return *result;

        if (auto result = abbreviate_prefix("Filter", "flt")) return *result;

        if (auto result = abbreviate_prefix("String", "str")) return *result;

        if (auto result = abbreviate_prefix("Soft Clip", "clp")) return *result;

        if (auto result = abbreviate_prefix("Delay", "dly")) return *result;

        if (auto result = abbreviate_prefix("Noise", "ns")) return *result;

        if (auto result = abbreviate_prefix("Lane", "ln")) return *result;

        return label;
    }
}


AudioRoutingManager::AudioRoutingManager() : SynthSection("audio_routing_manager"),
drag_icon_("audio_drag_icon"), mapping_mode_dim_quad_(Shaders::kColorFragment, "audio_mapping_mode_dim") {

    setInterceptsMouseClicks(false, true);

    mapping_mode_dim_quad_.setTargetComponent(this);
    mapping_mode_dim_quad_.setQuad(0,-1.0f,-1.0f,2.0f, 2.0f);
    mapping_mode_dim_quad_.setAlpha(0.0f);
    mapping_mode_dim_quad_.setInterceptsMouseClicks(false, false);

    drag_icon_.setShape(Paths::rightArrow());
    drag_icon_.setUseAlpha(true);
    drag_icon_.setActive(false);
    drag_icon_.setInterceptsMouseClicks(false, false);
    addChildComponent(&drag_icon_);
}


void AudioRoutingManager::registerPort (AudioPortComponent& port) {
    const auto already_registered = std::any_of(ports_.begin(), ports_.end(),[&port] (const SafePort &existing) {
        return existing.getComponent() == &port;
    });

    if (already_registered) return; // if the port is already registered, return

    ports_.emplace_back(&port);   // add to vector of safepointers, ports_
    port.addListener(this);     // add listener to new port
    updatePortConnectionSlots();
}

void AudioRoutingManager::unregisterPort (AudioPortComponent& port){
    port.removeListener(this);

    std::erase_if(ports_, [&port] (const SafePort& existing) { // erase this port, and any other null pointers
        return existing == nullptr || existing.getComponent() == &port;
    });

    if (current_source_ == &port)
        endAudioMap();

    if (current_destination_.getComponent() == &port)
        current_destination_ = nullptr;
}

void AudioRoutingManager::audioPortDragStarted (AudioPortComponent* port, const juce::MouseEvent& event) {
    startDestinationMap(port, event);
}

void AudioRoutingManager::audioPortDragged (AudioPortComponent* port, const juce::MouseEvent& event) {
    audioDragged(event);
}

void AudioRoutingManager::audioPortDragEnded (AudioPortComponent* port, const juce::MouseEvent& event) {
    endAudioMap();
}

void AudioRoutingManager::startDestinationMap(AudioPortComponent* source, const juce::MouseEvent& event) {
    if (source == nullptr || (source->getAddress().direction != electrosynth::audio::PortDirection::Output)) return;
    current_source_ = source;
    current_destination_ = nullptr;
    dragging_ = true;

    mouse_drag_position_ = getLocalPoint(source, event.getPosition());
    positionDragIcon();

    for (const auto& safe_port : ports_) {
        if (auto* port = safe_port.getComponent()) {
            port->setMappingTarget(isValidDestination (port));
        }
    }
}

void AudioRoutingManager::drawDestinationHighlights(OpenGlWrapper& open_gl) {
    if (!isMappingMode())
        return;

    for (const auto& safe_port : ports_) {
        auto* port = safe_port.getComponent();
        if (isValidDestination (port)) port->render (open_gl, true);
    }

}

bool AudioRoutingManager::isMappingMode() const {
    return dragging_ && current_source_ != nullptr;
}

bool AudioRoutingManager::isPointInsideDestinationDropArea(AudioPortComponent* destination, juce::Point<int> position) const {
    if (!isValidDestination (destination))
        return false;

    const auto& source = current_source_->getAddress();
    const auto& target = destination->getAddress();

    if (target.direction != electrosynth::audio::PortDirection::Input || target.domain != source.domain || target.nodeId == source.nodeId
        || !destination->isShowing())
        return false;

    const auto screen_position = localPointToGlobal(position);

    const auto destination_position = destination->getLocalPoint(nullptr, screen_position);

    return destination->reallyContains(destination_position, false);
}

void AudioRoutingManager::audioDragged(const juce::MouseEvent& event) {
    if (!isMappingMode())
        return;

    mouse_drag_position_ = getLocalPoint(current_source_, event.getPosition());
    positionDragIcon();

    AudioPortComponent* destination = nullptr;

    for (const auto& safe_port : ports_) {
        auto* candidate_destination = safe_port.getComponent();

        if (isPointInsideDestinationDropArea(candidate_destination, mouse_drag_position_)) {
            destination = candidate_destination;
            break;
        }
    }

    audioDraggedToComponent(destination);
}

void AudioRoutingManager::audioDraggedToComponent(AudioPortComponent* destination) {
    if (current_destination_.getComponent() == destination)
        return;

    if (current_destination_) // current_destination_ is initialized to nullptr
        current_destination_->setDragTarget(false);

    current_destination_ = destination; // then it's set to whatever we are hovering over

    if (current_destination_) // if we're hovering over something, make it a drag target
        current_destination_->setDragTarget(true);

}

bool AudioRoutingManager::isValidDestination(const AudioPortComponent* port) const {
    if (port == nullptr || port == current_source_)
        return false;

    return port->getAddress().direction == electrosynth::audio::PortDirection::Input;
}

void AudioRoutingManager::positionDragIcon() {
    static constexpr float kIconWidthRatio = 0.03f;

    if (!isMappingMode() || getWidth() <= 0)
        return;

    const int icon_size = static_cast<int>(std::round(kIconWidthRatio * getWidth()));

    const juce::Rectangle<int> bounds(mouse_drag_position_.x - icon_size / 2, mouse_drag_position_.y - icon_size / 2,
        icon_size, icon_size);

    if (drag_icon_.getBounds() != bounds)
        drag_icon_.setBounds(bounds);

    drag_icon_.setColor(findColour(Skin::kWidgetPrimary1, true));

    drag_icon_.setActive(true);
    drag_icon_.setVisible(true);
    drag_icon_.redrawImage(true);
}

void AudioRoutingManager::endAudioMap() {
    if (current_source_ != nullptr && current_destination_ != nullptr) {
        electrosynth::audio::AudioConnection connection;
        connection.source = current_source_->getAddress();
        connection.destination = current_destination_->getAddress();
        if (connection.isValid())
            connectAudio(connection);
    }

    dragging_ = false;
    current_source_ = nullptr;
    current_destination_ = nullptr;

    drag_icon_.setVisible(false);
    drag_icon_.setActive(false);

    for (const auto& safe_port : ports_) {
        if (auto* port = safe_port.getComponent()) {
            port->setMappingTarget(false);
            port->setDragTarget(false);
        }
    }
}

void AudioRoutingManager::drawDraggingAudio(OpenGlWrapper& open_gl) {
    if (!isMappingMode())
        return;
    drag_icon_.render(open_gl, true);
}

void AudioRoutingManager::drawMappingMode(OpenGlWrapper& open_gl) {
    if (!isMappingMode()) {
        mapping_mode_dim_quad_.setAlpha(0.0f);
        return;
    }

    mapping_mode_dim_quad_.setColor(findColour(Skin::kBackground, true));
    mapping_mode_dim_quad_.setAlpha(0.45f);
    mapping_mode_dim_quad_.render(open_gl, true);
}

// OpenGl lifecycle methods *************************************************
void AudioRoutingManager::initOpenGlComponents(OpenGlWrapper& open_gl) {
    mapping_mode_dim_quad_.init(open_gl);
    drag_icon_.init(open_gl);
    SynthSection::initOpenGlComponents(open_gl);
}

void AudioRoutingManager::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
    juce::ScopedLock lock(open_gl_critical_section_);
    drawMappingMode(open_gl); // dark overlay first...
    SynthSection::renderOpenGlComponents(open_gl, animate);
    OpenGlComponent::setViewPort(this, open_gl);
    drawDestinationHighlights(open_gl); // then bright targets
    drawDraggingAudio(open_gl); // then bright icon
}

void AudioRoutingManager::destroyOpenGlComponents(juce::OpenGLContext& open_gl) {
    mapping_mode_dim_quad_.destroy(open_gl);
    drag_icon_.destroy(open_gl);
    SynthSection::destroyOpenGlComponents(open_gl);
}

bool AudioRoutingManager::connectAudio(const electrosynth::audio::AudioConnection& connection) {
    if (!connection.isValid()) return false;

    connections_.push_back(connection);
    updatePortConnectionSlots();
    return true;
}

std::optional<electrosynth::audio::AudioConnection> AudioRoutingManager::getConnectionTo(const electrosynth::audio::AudioPortAddress& destination) const {
    const auto match = std::find_if(connections_.begin(), connections_.end(), [&](const auto& connection) {
        return connection.destination.nodeId == destination.nodeId && connection.destination.portId == destination.portId;
    });
    if (match == connections_.end()) return std::nullopt;

    return *match;
}

void AudioRoutingManager::updatePortConnectionSlots() {
    const auto find_port = [this](const electrosynth::audio::AudioPortAddress& address) -> AudioPortComponent* {
        for (const auto& safe_port : ports_) {
            auto* port = safe_port.getComponent();

            if (port != nullptr && port->getAddress().nodeId == address.nodeId
                && port->getAddress().portId == address.portId)
                return port;
        }
        return nullptr;
    };

    for (const auto& safeport : ports_) { // iterate through all ports

        auto* port = safeport.getComponent();
        if (port == nullptr) continue;

        const auto& address = port->getAddress();
        std::vector<AudioConnectionSlot> slots_for_port;

        for (const auto& connection : connections_) { // for each port, iterate through all of its connections

            const auto& endpoint = address.direction == electrosynth::audio::PortDirection::Input ?
                    connection.destination : connection.source;

            if (endpoint.nodeId == address.nodeId && endpoint.portId == address.portId) {

                const auto& peer_address = address.direction == electrosynth::audio::PortDirection::Input ?
                    connection.source : connection.destination;

                if (auto* peer = find_port(peer_address)) {

                    auto* owner = peer->getParentComponent();

                    const auto full_label = owner != nullptr && owner->getName().isNotEmpty() ?
                        owner->getName() : peer->getName();
                    const auto label = abbreviateLabel (full_label); 

                    slots_for_port.push_back({peer_address,label,
                        peer->findColour(Skin::kWidgetPrimary1, true)
                    });
                }
            }
        }

        if (auto* slots = port->getConnectionSlots())
            slots->setConnections(std::move(slots_for_port));
    }
}
