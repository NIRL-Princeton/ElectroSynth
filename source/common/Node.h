//
// Created by Callista Chong on 7/16/26.
//

#pragma once

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


    struct NodeDescriptor // defines one component's complete routing capabilities, will later contain multiple port adresses
    {
        NodeType type = NodeType::Processor;
        AudioDomain domain = AudioDomain::PerVoiceStereo;
        bool hasInput = false;
        bool hasOutput = false;
        juce::String inputPortId = { "audio_in"};
        juce::String outputPortId = { "audio_out"};
    };

    inline NodeDescriptor makeGeneratorDescriptor()
    {
        return {
        .type = NodeType::Generator,
        .domain = AudioDomain::PerVoiceStereo,
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
