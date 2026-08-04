//
// Created by Matthew McWeeney on 8/3/26.
//

#ifndef ELECTORSYNTH_SINEMODULEPROCESSOR_H
#define ELECTORSYNTH_SINEMODULEPROCESSOR_H

#include "SineModule.h"
#include "../PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "ProcessorBase.h"
//#include "leaf-midi.h"
// namespace electrosynth{
//     namespace utils
//     {
//         float stringToHarmonicVal(const juce::String &s);
//         juce::String harmonicValToString(float harmonic);
//     }
// }

struct SineOscParams : public LEAFParams<_tSineModule >
{
    SineOscParams(LEAF* leaf) : LEAFParams<_tSineModule>(leaf)
    {
       add(gain);
    }
    //add env watch param so that it isnt null
    chowdsp::FloatParameter::Ptr envwatchparam {
        juce::ParameterID { "watch", 100 },
        "watch",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        1.0f,
        all_params[0],
        [this] (float val) {
            // for (auto mod: modules) mod->setterFunctions[EnvParams::EnvSustain](mod, val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::GainDBParameter::Ptr gain
    {
        juce::ParameterID{"gain" , 100},
        "Gain",
        chowdsp::ParamUtils::createNormalisableRange(-24.f, 24.f ,0.f),
        0.f,
        all_params[SineParams::SineGain],
        [this]( float val)
        {for (auto mod : modules)
            tSineModule_setParameter(mod,SineGain,val);
        }
    };

};
class SineModuleProcessor : public ProcessorStateBase<PluginStateImpl_<SineOscParams>>
{
public:
    SineModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf,juce::UndoManager*);

    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
    void releaseResources() override {}
    //void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
//    bool acceptsMidi() const override
//    {
//       return true;
    std::unique_ptr<SynthSection> createEditor() override
    {
        return std::make_unique<electrosynth::ParametersView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
   // juce::AudioProcessorEditor* createEditor() override {return new electrosynth::ParametersViewEditor{*this,vstate.getProperty(IDs::type).toString() + vstate.getProperty(IDs::uuid).toString()};};
    chowdsp::ScopedCallbackList callbacks;

    //uint8_t noVoicesSounding = 1;
};

#endif // ELECTORSYNTH_SINEMODULEPROCESSOR_H
