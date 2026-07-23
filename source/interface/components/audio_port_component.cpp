//
// Created by Callista Chong on 7/22/26.
//

#include "audio_port_component.h"

#include "juce_events/juce_events.h"

AudioPortComponent::AudioPortComponent(juce::String name, electrosynth::audio::AudioPortAddress address)
    : PlainShapeComponent(std::move(name)), address_(std::move(address)) {
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
    setArrowScale(1.0f);
}

void AudioPortComponent::mouseExit(const juce::MouseEvent& event) {
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
