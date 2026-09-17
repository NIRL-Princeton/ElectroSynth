//
// Created by Callista Chong on 7/16/26.
//

#pragma once

#include <algorithm>
#include <vector>

#include <juce_core/juce_core.h>
#include "Identifiers.h"

namespace electrosynth::audio
{
    enum class NodeType // describes what a node does
    {
        Generator,
        Processor,
        SystemProcessor,
        Sink
    };

    enum class AudioDomain // describes buffer format
    {
        PerVoiceStereo
    };

    enum class EndpointKind
    {
        Audio,
        Parameter
    };

    enum class PortDirection
    {
        Input,
        Output
    };

    struct EndpointDescriptor
    {
        juce::String id;
        EndpointKind kind = EndpointKind::Audio;
        PortDirection direction = PortDirection::Input;
        AudioDomain domain = AudioDomain::PerVoiceStereo;

        bool hasAmount = false;
        bool hasBipolar = false;
        bool hasStereo = false;
        int maxConnections = 1;
    };

    struct NodeDescriptor // defines one component's complete routing capabilities, will later contain multiple port adresses
    {
        NodeType type = NodeType::Processor;
        AudioDomain domain = AudioDomain::PerVoiceStereo;
        juce::String nodeId;
        juce::String displayName;

        std::vector<EndpointDescriptor> endpoints;

        // Legacy compatibility while the UI/router migrates to endpoint lists.
        bool hasInput = false;
        bool hasOutput = false;
        juce::String inputPortId = { "audio_in"};
        juce::String outputPortId = { "audio_out"};

        bool supportsInput() const noexcept {
            return hasInput || std::any_of(endpoints.begin(), endpoints.end(),
                [] (const auto& endpoint) {
                    return endpoint.direction == PortDirection::Input;
                });
        }

        bool supportsOutput() const noexcept {
            return hasOutput || std::any_of(endpoints.begin(), endpoints.end(),
                [] (const auto& endpoint) {
                    return endpoint.direction == PortDirection::Output;
                });
        }

        const EndpointDescriptor* findEndpoint(const juce::String& endpointId,
                                               PortDirection direction = PortDirection::Input) const noexcept
        {
            const auto it = std::find_if(endpoints.begin(), endpoints.end(),
                [&] (const auto& endpoint) {
                    return endpoint.id == endpointId && endpoint.direction == direction;
                });
            return it != endpoints.end() ? &*it : nullptr;
        }
    };

    inline NodeDescriptor makeGeneratorDescriptor()
    {
        return {
        .type = NodeType::Generator,
        .domain = AudioDomain::PerVoiceStereo,
        .nodeId = {},
        .displayName = {},
        .endpoints = {
            { "audio_out", EndpointKind::Audio, PortDirection::Output, AudioDomain::PerVoiceStereo, false, false, false, 1 }
        },
        .hasInput = false,
        .hasOutput= true,
        .inputPortId = "audio_in",
        .outputPortId = "audio_out"};
    }

    inline NodeDescriptor makeProcessorDescriptor()
    {
        return {
            .type = NodeType::Processor,
            .domain = AudioDomain::PerVoiceStereo,
            .nodeId = {},
            .displayName = {},
            .endpoints = {
                { "audio_in", EndpointKind::Audio, PortDirection::Input, AudioDomain::PerVoiceStereo, false, false, false, 1 },
                { "audio_out", EndpointKind::Audio, PortDirection::Output, AudioDomain::PerVoiceStereo, false, false, false, 1 }
            },
            .hasInput = true,
            .hasOutput = true,
            .inputPortId = "audio_in",
            .outputPortId = "audio_out"
        };
    }

    inline NodeDescriptor makeSystemProcessorDescriptor()
    {
        return {
            .type = NodeType::SystemProcessor,
            .domain = AudioDomain::PerVoiceStereo,
            .nodeId = {},
            .displayName = {},
            .endpoints = {
                { "audio_in", EndpointKind::Audio, PortDirection::Input, AudioDomain::PerVoiceStereo, false, false, false, 1 },
                { "audio_out", EndpointKind::Audio, PortDirection::Output, AudioDomain::PerVoiceStereo, false, false, false, 1 }
            },
            .hasInput = true,
            .hasOutput = true,
            .inputPortId = "audio_in",
            .outputPortId = "audio_out"
        };
    }

    inline juce::String createNodeId() {
        return juce::Uuid().toString();
    }

    inline void ensureNodeId(juce::ValueTree state, juce::UndoManager* undo_manager = nullptr) {
        if (!state.isValid()) {
            jassertfalse;
            return;
        }

        auto nodeId = state.getProperty (IDs::nodeID).toString();
        if (nodeId.isEmpty()) nodeId = state.getProperty(IDs::audioNodeId).toString();
        if (nodeId.isEmpty()) nodeId = createNodeId();

        state.setProperty(IDs::nodeID, nodeId, undo_manager);
    }

}
