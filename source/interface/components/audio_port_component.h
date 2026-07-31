//
// Created by Callista Chong on 7/21/26.
//

#pragma once

#include "ConnectionRecord.h"
#include "endpoint_arrow_component.h"

class ConnectionSlots;

// this is the visible, clickable arrow. All this does is know its endport and maintains slots.

class AudioPortComponent : public EndpointArrowComponent {

public:

    AudioPortComponent(juce::String name, electrosynth::EndpointDescriptor endpoint);

    const electrosynth::EndpointDescriptor& getEndpoint() const noexcept {
        return endpoint_;
    }

    void resized() override {
        PlainShapeComponent::resized();
        redrawImage(true);
    }

    void setConnectionSlots(ConnectionSlots* slots) noexcept;
    ConnectionSlots* getConnectionSlots() const noexcept;

private:
    electrosynth::EndpointDescriptor endpoint_;
    ConnectionSlots* connection_slots_ = nullptr;
};
