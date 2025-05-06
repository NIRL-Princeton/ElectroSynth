//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_MODULATORBASE_H
#define ELECTROSYNTH_MODULATORBASE_H
#include "PluginStateImpl_.h"
#include "leaf.h"
#include "ParameterView/ParametersView.h"
namespace electrosynth {
    class SoundEngine;
}
class ModulatorBase : public juce::AudioSource
{
public:
    explicit ModulatorBase( electrosynth::SoundEngine* engine,LEAF* leaf,juce::ValueTree& tree, juce::UndoManager* um = nullptr) :
        engine(engine),
        leaf(leaf),
        vt(tree)
    {

    }
    ~ModulatorBase() override = default;
    LEAF* leaf;
    juce::ValueTree vt;
    leaf::Processor* procArray;
    juce::String name;
    virtual void process();
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    virtual electrosynth::ParametersView* createEditor() = 0;
    electrosynth::SoundEngine* engine;
};


template <typename PluginStateType>
class ModulatorStateBase : public ModulatorBase{
public :
    ModulatorStateBase(electrosynth::SoundEngine* engine, LEAF* leaf, juce::ValueTree& tree, juce::UndoManager* um = nullptr)
    : ModulatorBase(engine, leaf, tree, um),
          state(leaf)
    {

    }
    PluginStateType state;
};

#endif //ELECTROSYNTH_MODULATORBASE_H
