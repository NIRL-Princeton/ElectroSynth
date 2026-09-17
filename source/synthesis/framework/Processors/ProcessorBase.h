//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_PROCESSORBASE_H
#define ELECTROSYNTH_PROCESSORBASE_H
#include "ModuleBase.h"
#include "ParameterView/ParametersView.h"
#include "PluginStateImpl_.h"
#include "leaf.h"

namespace electrosynth {
    class SoundEngine;
}
class ProcessorBase : public ModuleBase
{
public:
    explicit ProcessorBase(electrosynth::SoundEngine* engine, LEAF* leaf, const juce::ValueTree& tree, juce::UndoManager* um = nullptr)
        : ModuleBase(engine, leaf, tree) {}

    ~ProcessorBase() override = default;

    virtual void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) = 0;
    virtual electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept = 0; // expose AudioNode
    virtual void getStateInformation (MemoryBlock &destData)=0;
    virtual void setStateInformation (const void *data, int sizeInBytes)=0;
};


template <typename PluginStateType>
class ProcessorStateBase : public ProcessorBase {
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
