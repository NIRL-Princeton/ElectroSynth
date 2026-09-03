//
// Created by Mstthew McWeeney on 9/3/26.
//

#ifndef ELECTORSYNTH_SIMPLEENVMODULEPROCESSOR_H
#define ELECTORSYNTH_SIMPLEENVMODULEPROCESSOR_H

#include "ModulatorBase.h"
#include "PluginStateImpl_.h"
#include "Identifiers.h"
#include "SimpleEnvModule.h"

struct SimpEnvParamHolder : public LEAFParams<_tSimpleEnvModule>
{
    SimpEnvParamHolder(LEAF* leaf) : LEAFParams<_tSimpleEnvModule>(leaf)
    {
        add(velocityParam,
            attackParam,
            decayParam,
            sustainParam,
            releaseParam,
            leakParam
            );
    }

    chowdsp::FloatParameter::Ptr envwatchparam {
        juce::ParameterID { "watch", 100 },
        "watch",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        1.0f,
        all_params[SimpEnvEventWatchFlag],
        [this] (float val) {
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
    chowdsp::FloatParameter::Ptr velocityParam {
        juce::ParameterID { "velocity", 100 },
        "Velocity Sensitivity",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.9f),
        1.0f,
        all_params[SimpEnvVelocitySense],
        [this] (float val) {
            for (auto mod: modules) tSimpleEnvModule_setParameter(mod, SimpEnvVelocitySense, val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Attack param
    chowdsp::TimeMsParameter::Ptr attackParam
    {
        juce::ParameterID { "attack", 100 },
            "Attack",
            chowdsp::ParamUtils::createNormalisableRange (0.0f, 20000.0f, 4000.f),
            0.005f,
            all_params[SimpEnvAttack],
            [this] (float val) {
                for (auto mod: modules)tSimpleEnvModule_setParameter(mod,SimpEnvAttack,val);
            }
    };



    // Decay param
    chowdsp::TimeMsParameter::Ptr decayParam {
        juce::ParameterID { "decay", 100 },
        "Decay",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 20000.0f, 4000.f),
        0.3f,
        all_params[SimpEnvDecay],
        [this] (float val) {
            for (auto mod: modules)tSimpleEnvModule_setParameter(mod,SimpEnvDecay,val);
        }
    };


    // Sustain param
    chowdsp::FloatParameter::Ptr sustainParam {
        juce::ParameterID { "sustain", 100 },
        "Sustain",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        1.0f,
        all_params[SimpEnvSustain],
        [this] (float val) {
            for (auto mod: modules) tSimpleEnvModule_setParameter(mod,SimpEnvSustain,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Release param
    chowdsp::TimeMsParameter::Ptr releaseParam {
        juce::ParameterID { "release", 100 },
        "Release",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 20000.0f, 4000.f),
        3.f,
        all_params[SimpEnvRelease],
        [this] (float val) {
            for (auto mod: modules) tSimpleEnvModule_setParameter(mod,SimpEnvRelease,val);
        }
    };

    // Leak param
    chowdsp::FloatParameter::Ptr leakParam {
        juce::ParameterID { "leak", 100 },
        "Leak",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.0f,
        all_params[SimpEnvLeak],
        [this] (float val) {
            for (auto mod: modules) tSimpleEnvModule_setParameter(mod,SimpEnvLeak,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Shape param
    // chowdsp::FloatParameter::Ptr shapeParam {
    //     juce::ParameterID { "shape", 100 },
    //     "Shape",
    //     chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
    //     0.0f,
    //     all_params[SimpEnvShape],
    //     [this] (float val) {
    //         for (auto mod: modules) tSimpleEnvModule_setParameter(mod,SimpEnvShape,val);
    //     },
    //     &chowdsp::ParamUtils::floatValToString,
    //     &chowdsp::ParamUtils::stringToFloatVal
    // };


};



class SimpleEnvModuleProcessor : public ModulatorStateBase<PluginStateImpl_<SimpEnvParamHolder>>
{
public:
    SimpleEnvModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree&, LEAF* leaf, juce::UndoManager*);
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    juce::AudioBuffer<float>* processMasterEnvelope();
    std::unique_ptr<SynthSection> createEditor() override
    {
        return std::make_unique<electrosynth::ParametersView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
    void process() override;
};

#endif // ELECTORSYNTH_SIMPLEENVMODULEPROCESSOR_H
