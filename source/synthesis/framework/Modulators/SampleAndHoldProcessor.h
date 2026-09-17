//
// Created by Matthew McWeeney on 8/28/26.
//

#ifndef ELECTORSYNTH_SAMPLEANDHOLDPROCESSOR_H
#define ELECTORSYNTH_SAMPLEANDHOLDPROCESSOR_H

#include "SampleAndHoldModule.h"
#include "../PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "ModulatorBase.h"
#include "../Processors/ProcessorBase.h"
#include "ParameterView/FxModuleTemplateView.h"
#include "PluginStateImpl_.h"

namespace electrosynth{
    namespace utils
    {
        float stringToHarmonicVal2(const juce::String &s);
        juce::String harmonicValToString2(float harmonic);
    }
}

struct SampHoldParamHolder : public LEAFParams<_tSampleAndHoldModule>
{
    SampHoldParamHolder(LEAF* leaf) : LEAFParams(leaf)
    {
        add(threshold, frequency, keyFollow, harmonic, durRand, gain, mix);
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

    chowdsp::GainDBParameter::Ptr threshold {
        juce::ParameterID { "threshold", 100 },
        "Threshold",
        chowdsp::ParamUtils::createNormalisableRange (-10000.f,12.f,0.f),
        0.0f,
        all_params[SampHoldParams::SampHoldThreshold],
        [this] (float val) {
            for (auto mod: modules) tSampleAndHoldModule_setParameter(mod,SampHoldParams::SampHoldThreshold,val);
        }
    };

    // chowdsp::FloatParameter::Ptr triggerToggle {
    //     juce::ParameterID { "triggerToggle", 100 },
    //     "TriggerToggle",
    //     chowdsp::ParamUtils::createNormalisableRange (0.f,1.0f,.5f, 1.f),
    //     0.f,
    //     all_params[SampHoldParams::SampHoldTriggerToggle],
    //     [this] (float val) {
    //         for (auto mod: modules) tSampleAndHoldModule_setParameter(mod,SampHoldParams::SampHoldTriggerToggle,val);
    //     },
    //     &chowdsp::ParamUtils::floatValToString,
    //     &chowdsp::ParamUtils::stringToFloatVal
    // };

    chowdsp::FreqHzParameter::Ptr frequency {
        juce::ParameterID { "frequency", 100 },
        "Frequency",
        chowdsp::ParamUtils::createNormalisableRange (0.f,20000.f,20.f),
        0.f,
        all_params[SampHoldParams::SampHoldFrequency],
        [this] (float val) {
            for (auto mod: modules) tSampleAndHoldModule_setParameter(mod,SampHoldParams::SampHoldFrequency,val);
        }
    };

    chowdsp::FloatParameter::Ptr keyFollow {
        juce::ParameterID { "keyFollow", 100 },
        "KeyFollow",
        chowdsp::ParamUtils::createNormalisableRange (0.f,1.0f,.5f),
        0.f,
        all_params[SampHoldParams::SampHoldKeyFollow],
        [this] (float val) {
            for (auto mod: modules) tSampleAndHoldModule_setParameter(mod,SampHoldParams::SampHoldKeyFollow,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr harmonic {
        juce::ParameterID{"harmonic" , 100},
        "Harmonic",
        chowdsp::ParamUtils::createNormalisableRange(-15.f, 15.f, 0.f, 1.f),
        0.f,
        all_params[SampHoldParams::SampHoldHarmonic],
        [this](float val){
            for (auto mod : modules)
                tSampleAndHoldModule_setParameter(mod,SampHoldHarmonic,val);
            //DBG("harm [0 - 1]" + juce::String(val) + " .. .  harm actual Val" + juce::String(modules[0]->harmonicMultiplier));
        },
        &electrosynth::utils::harmonicValToString2,
        &electrosynth::utils::stringToHarmonicVal2
    };

    chowdsp::FloatParameter::Ptr durRand {
        juce::ParameterID { "durRand", 100 },
        "DurRand",
        chowdsp::ParamUtils::createNormalisableRange (0.f,1.0f,.5f),
        0.f,
        all_params[SampHoldParams::SampHoldDurRand],
        [this] (float val) {
            for (auto mod: modules) tSampleAndHoldModule_setParameter(mod,SampHoldParams::SampHoldDurRand,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::GainDBParameter::Ptr gain {
        juce::ParameterID { "gain", 100 },
        "Gain",
        chowdsp::ParamUtils::createNormalisableRange (-10000.f,12.f,0.f),
        0.0f,
        all_params[SampHoldParams::SampHoldGain],
        [this] (float val) {
            for (auto mod: modules) tSampleAndHoldModule_setParameter(mod,SampHoldParams::SampHoldGain,val);
        }
    };

    // this is where the knob labeled "Mix" is created
    chowdsp::PercentParameter::Ptr mix {
        juce::ParameterID{"mix", 100},
        "Mix",
        all_params[SampHoldParams::SampHoldMix],
        [this](float val)
        {for (auto mod: modules) tSampleAndHoldModule_setParameter(mod,SampHoldMix,val);
        },
        1.f,
        false
    };
};

class SampleAndHoldProcessor : public ProcessorStateBase<PluginStateImpl_<SampHoldParamHolder>>
{
public:
    SampleAndHoldProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf,juce::UndoManager*);
    electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept override {
        return electrosynth::audio::makeProcessorDescriptor();
    }
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
    void releaseResources() override {}
    std::unique_ptr<SynthSection> createEditor() override {
        auto name = state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString();
        // module, vertical FxModuleTemplateView as an effect module (FX panel).
        if (state.hasType(IDs::SOUNDMODULE))
            return std::make_unique<electrosynth::ParametersView>(state_, state_.params, name);
        return std::make_unique<electrosynth::FxModuleTemplateView>(state_, state_.params, name);
    }
};

#endif // ELECTORSYNTH_SAMPLEANDHOLDPROCESSOR_H
