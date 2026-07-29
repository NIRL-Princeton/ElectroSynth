//
// Created by Callista Chong on 7/22/26.
//

#include "audio_port_component.h"
#include "juce_events/juce_events.h"
#include "audio_connection_slots.h"

AudioPortComponent::AudioPortComponent(juce::String name, electrosynth::audio::AudioPortAddress address)
    : PlainShapeComponent(std::move(name)), address_(std::move(address)){
    using electrosynth::audio::PortDirection;

    setShape(Paths::rightArrow());
    setArrowScale(0.8f);
    setActive(true);
    setUseAlpha(true);
    setInterceptsMouseClicks(true, false);
}

void AudioPortComponent::mouseDown(const juce::MouseEvent& event) {
    listeners_.call([&](Listener& listener) {
        listener.audioPortDragStarted(this, event);
    });
}

void AudioPortComponent::mouseEnter(const juce::MouseEvent& event) {
    mouse_hovered_ = true;
    updateArrowScale();
}

void AudioPortComponent::mouseExit(const juce::MouseEvent& event) {
    mouse_hovered_ = false;
    setArrowScale(0.8f);
}

void AudioPortComponent::mouseDrag(const juce::MouseEvent& event) {
    listeners_.call([&](Listener& listener) {
        listener.audioPortDragged(this, event);
    });
}

void AudioPortComponent::mouseUp(const juce::MouseEvent& event) {
    listeners_.call([&](Listener& listener) {
            listener.audioPortDragEnded(this, event);
        });
}

void AudioPortComponent::setArrowScale(float scale) {
    image().setTopLeft(-scale, scale);
    image().setTopRight(scale, scale);
    image().setBottomLeft(-scale, -scale);
    image().setBottomRight(scale, -scale);
}

void AudioPortComponent::setConnectionSlots(AudioConnectionSlots* slots) noexcept {
    connection_slots_ = slots;
}

AudioConnectionSlots* AudioPortComponent::getConnectionSlots() const noexcept {
    return connection_slots_;
}

void AudioPortComponent::setMappingTarget(bool target) {
    mapping_target_ = target;
    updateArrowScale();
}

void AudioPortComponent::setDragTarget(bool target) {
    drag_target_ = target;
    updateArrowScale();
}

void AudioPortComponent::updateArrowScale() {
    const float scale = drag_target_ || mouse_hovered_ ? 1.0f : mapping_target_ ? 0.9f : 0.8f;
    if (address_.direction == electrosynth::audio::PortDirection::Output)
        setArrowScale(scale);
}

void AudioPortComponent::render(OpenGlWrapper& open_gl, bool animate) {
    const auto color = drag_target_ ?
        findColour(Skin::kWidgetPrimary1, true) : mapping_target_ ?
        findColour(Skin::kTextComponentText, true) : findColour(Skin::kWidgetPrimary1, true);

    setColor(color);
    PlainShapeComponent::render(open_gl, animate);
}