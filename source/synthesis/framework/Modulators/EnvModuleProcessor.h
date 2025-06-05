//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_ENVMODULEPROCESSOR_H
#define ELECTROSYNTH_ENVMODULEPROCESSOR_H
#include "ModulatorBase.h"
//
// Created by Davis Polito on 11/19/24.
//
    #include "PluginStateImpl_.h"
#include "Identifiers.h"

struct EnvParamHolder : public LEAFParams<_tEnvModule>
{
    EnvParamHolder(LEAF* leaf) : LEAFParams<_tEnvModule>(leaf)
    {
        add(decayParam,
            sustainParam,
            releaseParam,
            attackParam
            );
    }

    // Sustain param
    chowdsp::FloatParameter::Ptr envwatchparam {
        juce::ParameterID { "watch", 100 },
        "watch",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        1.0f,
        all_params[EnvEventWatchFlag],
        [this] (float val) {
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Attack param
    chowdsp::TimeMsParameter::Ptr attackParam
    {
        juce::ParameterID { "attack", 100 },
            "Attack",
            chowdsp::ParamUtils::createNormalisableRange (0.0f, 10000.0f, 500.0f),
            1.0f,
            all_params[EnvParams::EnvAttack],
            [this] (float val) {
                for (auto mod: modules) mod->setterFunctions[EnvParams::EnvAttack](mod, val);
            }
    };





    // Decay param
    chowdsp::TimeMsParameter::Ptr decayParam {
        juce::ParameterID { "decay", 100 },
        "Decay",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1000.0f, 500.0f),
        0.0f,
        all_params[EnvParams::EnvDecay],
        [this] (float val) {
            for (auto mod: modules) mod->setterFunctions[EnvParams::EnvDecay](mod, val);
        }
    };


    // Sustain param
    chowdsp::FloatParameter::Ptr sustainParam {
        juce::ParameterID { "sustain", 100 },
        "Sustain",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        1.0f,
        all_params[EnvParams::EnvSustain],
        [this] (float val) {
            for (auto mod: modules) mod->setterFunctions[EnvParams::EnvSustain](mod, val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Release param
    chowdsp::TimeMsParameter::Ptr releaseParam {
        juce::ParameterID { "release", 100 },
        "Release",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1000.0f, 500.0f),
        50.0f,
        all_params[EnvParams::EnvRelease],
        [this] (float val) {
            for (auto mod: modules) mod->setterFunctions[EnvParams::EnvRelease](mod, val);
        }
    };

};



class EnvModuleProcessor : public ModulatorStateBase<PluginStateImpl_<EnvParamHolder>>
{
public:
    EnvModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree&, LEAF* leaf);
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    juce::AudioBuffer<float>* processMasterEnvelope();
    electrosynth::ParametersView* createEditor() override
    {
        return new electrosynth::ParametersView(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
};

#endif //ELECTROSYNTH_ENVMODULEPROCESSOR_H
