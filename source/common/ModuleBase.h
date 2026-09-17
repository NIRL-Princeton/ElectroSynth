//
// Created by Mike Mulshine on 8/15/26.
//

#pragma once

#include <array>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "Node.h"
#include "Identifiers.h"
#include "leaf.h"
#include "defs.h"

namespace electrosynth {
    class SoundEngine;
}

class LEAF;
class SynthSection;

class ModuleBase : public juce::AudioSource
{
public:
    ModuleBase(electrosynth::SoundEngine* engineIn, LEAF* leafIn, juce::ValueTree tree)
        : engine(engineIn), leaf(leafIn), state(tree)
    {
        electrosynth::audio::ensureNodeId(state, nullptr);
    }

    virtual ~ModuleBase() = default;

    juce::String getNodeId() const {
        return state.getProperty(IDs::nodeID).toString();
    }

    juce::String getDisplayName() const {
        return name;
    }

    virtual electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept = 0;

    // One-sample evaluation hook. Default no-op so block-based modules can
    // continue to compile while the routing graph is being unified.
    virtual void tick() {}

    virtual std::unique_ptr<SynthSection> createEditor() = 0;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override {}
    void prepareToPlay(int, double) override {}
    void releaseResources() override {}

    //protected:
    electrosynth::SoundEngine* engine = nullptr;
    LEAF* leaf = nullptr;
    juce::ValueTree state;
    std::array<ModuleHeader*, MAX_NUM_VOICES>* procArray = nullptr;
    juce::String name;
};
