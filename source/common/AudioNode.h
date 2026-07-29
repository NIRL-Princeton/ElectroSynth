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

    enum class PortDirection // describes direction
    {
        Input,
        Output
    };

    enum class AudioDomain // describes buffer format
    {
        PerVoiceStereo
    };

    struct PortDescriptor // defines one specific kind of connection to a node, such as audio_in
    {
        juce::String Id;
        juce::String displayName;
        PortDirection direction = PortDirection::Input;
        AudioDomain domain = AudioDomain::PerVoiceStereo;
    };

    struct NodeDescriptor // defines one component's complete routing capabilities, will later contain multiple port adresses
    {
        NodeType type = NodeType::Processor;
        AudioDomain domain = AudioDomain::PerVoiceStereo;
        bool hasInput = false;
        bool hasOutput = false;
        juce::String inputPortId = { "audioIn"};
        juce::String outputPortId = { "audioOut"};
    };

    inline NodeDescriptor makeGeneratorDescriptor()
    {
        return {
        .type = NodeType::Generator,
        .domain = AudioDomain::PerVoiceStereo,
        .hasInput = false,
        .hasOutput= true,
        .inputPortId = "audioIn",
        .outputPortId = "audioOut"};
    }

    inline NodeDescriptor makeProcessorDescriptor()
    {
        return {
            .type = NodeType::Processor,
            .domain = AudioDomain::PerVoiceStereo,
            .hasInput = true,
            .hasOutput = true,
            .inputPortId = "audioIn",
            .outputPortId = "audioOut"
        };
    }

    inline NodeDescriptor makeSystemProcessorDescriptor()
    {
        return {
            .type = NodeType::SystemProcessor,
            .domain = AudioDomain::PerVoiceStereo,
            .hasInput = true,
            .hasOutput = true,
            .inputPortId = "audioIn",
            .outputPortId = "audioOut"
        };
    }

    inline juce::String createAudioNodeId() {
        return juce::Uuid().toString();
    }

    inline void ensureAudioNodeId(juce::ValueTree state, juce::UndoManager* undo_manager = nullptr) {
        if (!state.isValid()) {
            jassertfalse;
            return;
        }

        const auto existingId = state.getProperty (IDs::audioNodeId).toString();
        if (existingId.isEmpty()) {
            state.setProperty(IDs::audioNodeId, createAudioNodeId(), undo_manager);
        }
    }

    // one port belonging to one specific node
    struct AudioPortAddress {
        juce::String nodeId;
        juce::String nodeName;
        juce::String portId;
        PortDirection direction = PortDirection::Input;
        AudioDomain domain = AudioDomain::PerVoiceStereo;

        bool isValid() const noexcept {
            return nodeId.isNotEmpty() && portId.isNotEmpty();
        }
    };


}