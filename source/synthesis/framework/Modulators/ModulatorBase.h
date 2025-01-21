//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_MODULATORBASE_H
#define ELECTROSYNTH_MODULATORBASE_H
#include "PluginStateImpl_.h"
#include "leaf.h"
#include "ParameterView/ParametersView.h"

class ModulatorBase : public juce::AudioSource
{
public:
    explicit ModulatorBase( LEAF* leaf,juce::ValueTree& tree, juce::UndoManager* um = nullptr) :

        leaf(leaf),
        vt(tree)
    {

    }
    ~ModulatorBase() override = default;
    LEAF* leaf;
    juce::ValueTree vt;
    leaf::Processor* proc;
    juce::String name;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    virtual electrosynth::ParametersView* createEditor() = 0;

};


template <typename PluginStateType>
class ModulatorStateBase : public ModulatorBase{
public :
    ModulatorStateBase(LEAF* leaf, juce::ValueTree& tree, juce::UndoManager* um = nullptr)
    : ModulatorBase(leaf, tree, um),
          state(leaf)
    {

    }
    PluginStateType state;
};

#endif //ELECTROSYNTH_MODULATORBASE_H
