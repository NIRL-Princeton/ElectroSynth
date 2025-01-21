//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_PROCESSORBASE_H
#define ELECTROSYNTH_PROCESSORBASE_H
#include "PluginStateImpl_.h"
#include "leaf.h"
#include "ParameterView/ParametersView.h"

class ProcessorBase : public juce::AudioSource
{
public:
    explicit ProcessorBase( LEAF* leaf,const juce::ValueTree& tree, juce::UndoManager* um = nullptr) :

        leaf(leaf),
        vt(tree)
    {

    }
    ~ProcessorBase() override = default;
    LEAF* leaf;
    juce::ValueTree vt;
    leaf::Processor* proc;
    juce::String name;
    virtual void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) = 0;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    virtual electrosynth::ParametersView* createEditor() = 0;

};


template <typename PluginStateType>
class ProcessorStateBase : public ProcessorBase{
public :
    ProcessorStateBase(LEAF* leaf, const juce::ValueTree& tree, juce::UndoManager* um = nullptr)
    : ProcessorBase(leaf, tree, um),
          state(leaf)
    {

    }
    PluginStateType state;
};

#endif //ELECTROSYNTH_PROCESSORBASE_H
