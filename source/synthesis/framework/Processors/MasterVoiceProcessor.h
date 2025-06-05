//
// Created by Davis Polito on 5/9/25.
//

#ifndef MASTERVOICEPROCESSOR_H
#define MASTERVOICEPROCESSOR_H
//
// Created by Davis Polito on 8/8/24.
//

#ifndef ELECTROSYNTH_MasterVoicePROCESSOR_H
#define ELECTROSYNTH_MasterVoicePROCESSOR_H
#include "SimpleOscModule.h"
#include "../PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "ProcessorBase.h"
#include "juce_graphics/fonts/harfbuzz/hb-wasm-api.h"
#include "Modulators/EnvModuleProcessor.h"

namespace electrosynth{
    namespace utils
    {
        float stringToHarmonicVal(const juce::String &s);
        juce::String harmonicValToString(float harmonic);
    }
}



struct MasterVoiceParams :  chowdsp::ParamHolder
{
    MasterVoiceParams(LEAF* leaf) : chowdsp::ParamHolder("MasterVoice" ),env(leaf)
    {

    }
    EnvParamHolder env;


};




class MasterVoiceProcessor : public ProcessorStateBase<PluginStateImpl_<MasterVoiceParams>>
{
public:
    MasterVoiceProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf);

    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
    void releaseResources() override {}
    //void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
//    bool acceptsMidi() const override
//    {
//       return true;
 electrosynth::ParametersView* createEditor() override
    {
        return new electrosynth::ParametersView(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
   // juce::AudioProcessorEditor* createEditor() override {return new electrosynth::ParametersViewEditor{*this,vstate.getProperty(IDs::type).toString() + vstate.getProperty(IDs::uuid).toString()};};
    chowdsp::ScopedCallbackList callbacks;

};

#endif //ELECTROSYNTH_MasterVoicePROCESSOR_H

#endif //MASTERVOICEPROCESSOR_H
