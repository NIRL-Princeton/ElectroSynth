//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_MODULATORBASE_H
#define ELECTROSYNTH_MODULATORBASE_H
#include "PluginStateImpl_.h"
#include "leaf.h"
#include "ParameterView/ParametersView.h"
#include "ModuleBase.h"

namespace electrosynth {
    class SoundEngine;
}

class ModulatorBase : public ModuleBase
{
public:
    explicit ModulatorBase( electrosynth::SoundEngine* engine,LEAF* leaf,juce::ValueTree& tree, juce::UndoManager* um = nullptr) :
        ModuleBase(engine, leaf, tree) {}

    ~ModulatorBase() override = default;

    electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept override {
        return electrosynth::audio::makeGeneratorDescriptor();
    }

    void tick() override {
        process();
    }

    virtual void process() = 0;
    virtual void getStateInformation (MemoryBlock &destData)=0;
    virtual void setStateInformation (const void *data, int sizeInBytes)=0;
};


template <typename PluginStateType>
class ModulatorStateBase : public ModulatorBase{
public :
    ModulatorStateBase(electrosynth::SoundEngine* engine, LEAF* leaf, juce::ValueTree& tree, juce::UndoManager* um = nullptr)
    : ModulatorBase(engine, leaf, tree, um),
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

#endif //ELECTROSYNTH_MODULATORBASE_H
