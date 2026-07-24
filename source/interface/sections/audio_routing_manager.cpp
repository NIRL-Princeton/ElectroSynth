//
// Created by Callista Chong on 7/22/26.
//

#include "audio_routing_manager.h"

#include "chowdsp_sources/chowdsp_sources.h"

AudioRoutingManager::AudioRoutingManager() : SynthSection("audio_routing_manager"), drag_icon_("audio_drag_icon") {
    setInterceptsMouseClicks(false, true);

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
}

bool AudioRoutingManager::isMappingMode() const {
    return dragging_ && current_source_ != nullptr;
}

bool AudioRoutingManager::isPointInsideDestinationDropArea(AudioPortComponent* destination, juce::Point<int> position) const {
    if (destination == nullptr || current_source_ == nullptr)
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

    current_destination_ = SafePort(destination);

    if (destination != nullptr) {
        DBG("Audio destination: " + destination->getAddress().nodeId + " / " + destination->getAddress().portId);
    }
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
        const auto& source = current_source_->getAddress();

        const auto& destination = current_destination_->getAddress();

        DBG("Audio connection: " + source.nodeId + "/" + source.portId + " -> " + destination.nodeId + "/" + destination.portId);
    }

    dragging_ = false;
    current_source_ = nullptr;
    current_destination_ = nullptr;

    drag_icon_.setVisible(false);
    drag_icon_.setActive(false);
}

// OpenGl lifecycle methods
void AudioRoutingManager::initOpenGlComponents(OpenGlWrapper& open_gl) {
    drag_icon_.init(open_gl);
    SynthSection::initOpenGlComponents(open_gl);
}

void AudioRoutingManager::drawDraggingAudio(OpenGlWrapper& open_gl) {
    if (!isMappingMode())
        return;
    drag_icon_.render(open_gl, true);
}

void AudioRoutingManager::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
    juce::ScopedLock lock(open_gl_critical_section_);

    SynthSection::renderOpenGlComponents(open_gl, animate);
    OpenGlComponent::setViewPort(this, open_gl);

    drawDraggingAudio(open_gl);
}

void AudioRoutingManager::destroyOpenGlComponents(juce::OpenGLContext& open_gl) {
    drag_icon_.destroy(open_gl);
    SynthSection::destroyOpenGlComponents(open_gl);
}