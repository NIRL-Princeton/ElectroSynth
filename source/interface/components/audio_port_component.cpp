//
// Created by Callista Chong on 7/22/26.
//

#include "audio_port_component.h"
#include "ConnectionRecord.h"
#include "connection_slots.h"
#include "juce_events/juce_events.h"

AudioPortComponent::AudioPortComponent(juce::String name, electrosynth::EndpointDescriptor endpoint)
    : EndpointArrowComponent(std::move(name)), endpoint_(std::move(endpoint)) {}

void AudioPortComponent::setConnectionSlots(ConnectionSlots* slots) noexcept {
    connection_slots_ = slots;
}

ConnectionSlots* AudioPortComponent::getConnectionSlots() const noexcept {
    return connection_slots_;
}
