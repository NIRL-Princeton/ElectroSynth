//
// Created by Matthew McWeeney on 7/9/26.
//

#ifndef ELECTORSYNTH_NOISEMODULEPROCESSOR_H
#define ELECTORSYNTH_NOISEMODULEPROCESSOR_H
#include "NoiseModule.h"
#include "../PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "ProcessorBase.h"

struct NoiseParams : public LEAFParams<_tNoiseModule>
{
    NoiseParams(LEAF* leaf) : LEAFParams<_tNoiseModule>(leaf)
    {
        add(gain, tilt, peakGain, peakFreq, peakBandwidth);
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

    // gain
    chowdsp::GainDBParameter::Ptr gain {
        juce::ParameterID{"gain", 100},
        "Gain",
        chowdsp::ParamUtils::createNormalisableRange(0.0f, 2.0f, 1.f),
        1.f,
        all_params[NosParams::NoiseGain],
        [this](float val)
        {for (auto mod: modules)    tNoiseModule_setParameter(mod,NoiseGain,val);
        },
    };

    // tilt
    chowdsp::FloatParameter::Ptr tilt {
        juce::ParameterID { "tilt", 100 },
        "Tilt",
        chowdsp::ParamUtils::createNormalisableRange (0.f, 1.0f, .5f),
        0.5f,
        all_params[NosParams::NoiseTilt],
        [this] (float val)
        {for (auto mod: modules) tNoiseModule_setParameter(mod, NoiseTilt,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // peakGain
    chowdsp::GainDBParameter::Ptr peakGain {
        juce::ParameterID{"peakGain", 100},
        "PeakGain",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 2.0f, 1.0f),
        1.f,
        all_params[NosParams::NoisePeakGain],
        [this](float val)
        {for (auto mod: modules)    tNoiseModule_setParameter(mod,NoisePeakGain,val);
        },
    };

    //
    chowdsp::FreqHzParameter::Ptr peakFreq {
        juce::ParameterID{"peakFreq" , 100},
        "PeakFreq",
        chowdsp::ParamUtils::createNormalisableRange(20.f, 20000.f, 500.f),
        500.f,
        all_params[NosParams::NoisePeakFreq],
        [this](float val)
        {
            for (auto mod: modules) tNoiseModule_setParameter(mod,NoisePeakFreq,val);
            //DBG("Noise [0 - 1]" + juce::String(val) + " .. .  peakFreq actual Val" + juce::String(modules[0]->peakFreq));
        }
    };

    chowdsp::FloatParameter::Ptr peakBandwidth {
        juce::ParameterID { "peakBandwidth", 100 },
        "PeakBandwidth",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        all_params[NosParams::NoisePeakBandwidth],
        [this] (float val) {
            for (auto mod: modules)
            {
                tNoiseModule_setParameter(mod,NoisePeakBandwidth,val);
                tNoiseModule_setParameter(mod, NoisePeakFreq, *mod->header.params[NoisePeakFreq]);
            }
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

};

class NoiseModuleProcessor : public ProcessorStateBase<PluginStateImpl_<NoiseParams>>
{
public:
    NoiseModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf,juce::UndoManager*);

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

};

#endif // ELECTORSYNTH_NOISEMODULEPROCESSOR_H
