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

    enum class EndpointDirection {
        Source,
        Destination
    };

    struct EndpointCapabilities {
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
        EndpointDirection direction = EndpointDirection::Source; // is this a source or destination?
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
        EndpointCapabilities capabilities;
    };

    struct ConnectionRecord {
        juce::String id;
        ConnectionType type = ConnectionType::Modulation; // what type is the source?
        EndpointAddress source;
        EndpointAddress destination;
        int destinationSlot = -1;   // which visual slot does this connection occupy?
        juce::String targetConnectionId; // non-empty when this modulates another connection's amount

        float amount = 1.0f; // persistent states for connection, not UI
        bool bipolar = false;
        bool bypass = false;
        bool stereo = false;

        bool isValid() const noexcept {
            return id.isNotEmpty() && source.isValid() && destination.isValid() && source.type == type &&
                destination.type == type && source.direction == EndpointDirection::Source
                && destination.direction == EndpointDirection::Destination;
        }
    };

    inline juce::String createConnectionRecordId() {
        return juce::Uuid().toString();
    }

    inline void writeConnectionRecordToTree(const ConnectionRecord& record, juce::ValueTree state) {
        state.setProperty(IDs::connectionId, record.id, nullptr);
        state.setProperty(IDs::connectionType, static_cast<int>(record.type), nullptr);
        state.setProperty(IDs::sourceNodeId, record.source.nodeId, nullptr);
        state.setProperty(IDs::sourceEndpointId, record.source.endpointId, nullptr);
        state.setProperty(IDs::destinationNodeId, record.destination.nodeId, nullptr);
        state.setProperty(IDs::destinationEndpointId, record.destination.endpointId, nullptr);
        state.setProperty(IDs::destIdx, record.destinationSlot, nullptr);
        state.setProperty(IDs::modAmt, record.amount, nullptr);
        state.setProperty(IDs::isBipolar, record.bipolar, nullptr);
        state.setProperty(IDs::bypass, record.bypass, nullptr);
        state.setProperty(IDs::stereo, record.stereo, nullptr);
        state.setProperty(IDs::audioDomain, static_cast<int>(record.source.audioDomain), nullptr);

        if (record.targetConnectionId.isNotEmpty())
            state.setProperty(IDs::targetConnectionId, record.targetConnectionId, nullptr);
        else
            state.removeProperty(IDs::targetConnectionId, nullptr);

        // Preserve the legacy modulation representation for older readers.
        if (record.type == ConnectionType::Modulation) {
            state.setProperty(IDs::src, record.source.endpointId, nullptr);
            state.setProperty(IDs::dest, record.destination.endpointId, nullptr);
        }
    }

    inline ConnectionRecord readConnectionRecordFromTree(const juce::ValueTree& state) {
        ConnectionRecord record;
        record.id = state.getProperty(IDs::connectionId).toString();
        if (record.id.isEmpty())
            record.id = createConnectionRecordId();

        record.type = state.hasType(IDs::MODULATION)
            ? ConnectionType::Modulation
            : static_cast<ConnectionType>(static_cast<int>(state.getProperty(
                IDs::connectionType, static_cast<int>(ConnectionType::Audio))));

        const auto sourceEndpoint = state.getProperty(
            IDs::sourceEndpointId, state.getProperty(IDs::src)).toString();
        const auto destinationEndpoint = state.getProperty(
            IDs::destinationEndpointId, state.getProperty(IDs::dest)).toString();
        const auto domain = static_cast<audio::AudioDomain>(static_cast<int>(state.getProperty(
            IDs::audioDomain, static_cast<int>(audio::AudioDomain::PerVoiceStereo))));

        record.source = {
            .type = record.type,
            .nodeId = state.getProperty(IDs::sourceNodeId).toString(),
            .endpointId = sourceEndpoint,
            .direction = EndpointDirection::Source,
            .audioDomain = domain
        };
        record.destination = {
            .type = record.type,
            .nodeId = state.getProperty(IDs::destinationNodeId).toString(),
            .endpointId = destinationEndpoint,
            .direction = EndpointDirection::Destination,
            .audioDomain = domain
        };
        record.destinationSlot = static_cast<int>(state.getProperty(IDs::destIdx, -1));
        record.amount = static_cast<float>(state.getProperty(IDs::modAmt, 1.0f));
        record.bipolar = static_cast<bool>(state.getProperty(IDs::isBipolar, false));
        record.bypass = static_cast<bool>(state.getProperty(IDs::bypass, false));
        record.stereo = static_cast<bool>(state.getProperty(IDs::stereo, false));
        record.targetConnectionId = state.getProperty(IDs::targetConnectionId).toString();
        return record;
    }
}
