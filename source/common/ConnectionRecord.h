//
// Created by Callista Chong on 7/29/26.
//

#pragma once
#include "Node.h"

namespace electrosynth {
    enum class ConnectionType {
        Modulation,
        Audio
    };

    enum class EndpointType {
        Source,
        Destination
    };

    struct EndpointFeature {
        bool hasAmount = false;
        bool hasBipolar = false;
        bool hasStereo = false;
        bool hasAuxiliary = false;
        int maxIncomingConnections = 3;
    };

    struct EndpointAddress {
        ConnectionType type = ConnectionType::Modulation;
        juce::String nodeId; // what node (or module, ex. filter 1.1, oscillation 3.2, etc.) does this endpoint belong to?
        juce::String endpointId; // which endpoint is this?
        EndpointType direction = EndpointType::Source; // is this a source or destination?
        audio::AudioDomain audioDomain = audio::AudioDomain::PerVoiceStereo;

        bool isValid() const noexcept {
            return nodeId.isNotEmpty() && endpointId.isNotEmpty();
        }

        bool matches(const EndpointAddress& other) const noexcept {
            return type == other.type && nodeId == other.nodeId
                && endpointId == other.endpointId && direction == other.direction;
        }
    };

    struct EndpointDescriptor {
        EndpointAddress address;
        EndpointFeature features;
    };

    struct ConnectionRecord {
        juce::String id;
        ConnectionType type = ConnectionType::Modulation; // what type is the source?
        EndpointAddress source;
        EndpointAddress destination;
        int destinationSlot = -1;   // which visual slot does this connection occupy?
        bool topologyDerived = false; // true if connection is result of simply being in graph (like module to module downward in lane)

        float amount = 1.0f;
        bool bipolar = false;
        bool bypass = false;
        bool stereo = false;

        bool isValid() const noexcept {
            const bool type_matches =
                (type == ConnectionType::Audio
                    && source.type == ConnectionType::Audio
                    && destination.type == ConnectionType::Audio)
                || (type == ConnectionType::Modulation
                    && destination.type == ConnectionType::Modulation
                    && (source.type == ConnectionType::Audio || source.type == ConnectionType::Modulation));

            return id.isNotEmpty() && source.isValid() && destination.isValid() && type_matches
                && source.direction == EndpointType::Source
                && destination.direction == EndpointType::Destination;
        }
    };

    inline juce::String createConnectionRecordId() {
        return juce::Uuid().toString();
    }
}
