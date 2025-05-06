//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_PROCESSORBASE_H
#define ELECTROSYNTH_PROCESSORBASE_H
#include "PluginStateImpl_.h"
#include "leaf.h"
#include "ParameterView/ParametersView.h"
namespace electrosynth {
    class SoundEngine;
}
class ProcessorBase : public juce::AudioSource
{
public:
    explicit ProcessorBase(electrosynth::SoundEngine* engine, LEAF* leaf,const juce::ValueTree& tree, juce::UndoManager* um = nullptr) :
        engine(engine),
        leaf(leaf),
        vt(tree)
    {

    }
    ~ProcessorBase() override = default;
    LEAF* leaf;
    juce::ValueTree vt;
    leaf::Processor* procArray;
    juce::String name;
    virtual void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&);
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    virtual electrosynth::ParametersView* createEditor() = 0;
    electrosynth::SoundEngine* engine;
};


template <typename PluginStateType>
class ProcessorStateBase : public ProcessorBase{
public :
    ProcessorStateBase(electrosynth::SoundEngine* engine,LEAF* leaf, const juce::ValueTree& tree, juce::UndoManager* um = nullptr)
    : ProcessorBase(engine,leaf, tree, um),
          state(leaf)
    {

    }
    PluginStateType state;
};

#endif //ELECTROSYNTH_PROCESSORBASE_H
