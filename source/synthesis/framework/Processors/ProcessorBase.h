//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_PROCESSORBASE_H
#define ELECTROSYNTH_PROCESSORBASE_H
#include "Identifiers.h"
#include "Node.h"
#include "ParameterView/ParametersView.h"
#include "PluginStateImpl_.h"
#include "leaf.h"

namespace electrosynth {
    class SoundEngine;
}
class ProcessorBase : public juce::AudioSource
{
public:
    explicit ProcessorBase(electrosynth::SoundEngine* engine, LEAF* leaf,const juce::ValueTree& tree, juce::UndoManager* um = nullptr) :
        engine(engine), leaf(leaf), state(tree) {
        electrosynth::audio::ensureNodeId(state, nullptr);
    }
    ~ProcessorBase() override = default;
    LEAF* leaf;
    juce::ValueTree state;
    std::array<ModuleHeader*, MAX_NUM_VOICES>* procArray;
    juce::String name;
    virtual void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) = 0;
    virtual electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept = 0; // expose AudioNode
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    virtual void getStateInformation (MemoryBlock &destData)=0;
    virtual void setStateInformation (const void *data, int sizeInBytes)=0;
    virtual std::unique_ptr<SynthSection> createEditor() = 0;
    electrosynth::SoundEngine* engine;
    juce::String getNodeId() const {
        return state.getProperty(IDs::nodeID).toString();
    }
};


template <typename PluginStateType>
class ProcessorStateBase : public ProcessorBase{
public :
    ProcessorStateBase(electrosynth::SoundEngine* engine,LEAF* leaf, const juce::ValueTree& tree, juce::UndoManager* um = nullptr)
    : ProcessorBase(engine,leaf, tree, um),
          state_(leaf,um)
    {
    state.setProperty(IDs::uuid, int(state_.params.headers[0]->uniqueID), nullptr);
    name = state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString();
    procArray = &state_.params.headers;
    if(state.isValid())
        chowdsp::Serialization::deserialize<chowdsp::XMLSerializer>(state.createXml(),state_);
    }
    PluginStateType state_;
    void getStateInformation(MemoryBlock &destData) override {
        state_.serialize(destData);
    }
    void setStateInformation (const void *data, int sizeInBytes) override {
        state_.deserialize (juce::MemoryBlock { data, (size_t) sizeInBytes });
    }
};

#endif //ELECTROSYNTH_PROCESSORBASE_H
