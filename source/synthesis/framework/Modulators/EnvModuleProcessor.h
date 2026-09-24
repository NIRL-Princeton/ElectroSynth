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
        add(velocityParam,
            attackParam,
            decayParam,
            sustainParam,
            releaseParam,
            leakParam,
            shapeParam

            );
    }

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
    chowdsp::FloatParameter::Ptr velocityParam {
        juce::ParameterID { "velocity", 100 },
        "Velocity Sensitivity",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.9f),
        1.0f,
        all_params[EnvVelocitySense],
        [this] (float val) {
            for (auto mod: modules) tEnvModule_setParameter(mod, EnvVelocitySense, val);
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
            3.0f,
            all_params[EnvParams::EnvAttack],
        [this] (float val) {
            const auto range = chowdsp::ParamUtils::createNormalisableRange(0.0f, 20000.f, 4000.f);
            const float ms = range.convertFrom0to1(val);
            for (auto mod : modules)
            {
                tEnvModule_setParameter(mod, EnvAttack, ms);
            }
        }
    };



    // Decay param
    chowdsp::TimeMsParameter::Ptr decayParam {
        juce::ParameterID { "decay", 100 },
        "Decay",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 20000.0f, 4000.f),
        3.0f,
        all_params[EnvParams::EnvDecay],
        [this] (float val) {
            const auto range = chowdsp::ParamUtils::createNormalisableRange(0.0f, 20000.f, 4000.f);
            const float ms = range.convertFrom0to1(val);
            for (auto mod : modules)
            {
                tEnvModule_setParameter(mod,EnvDecay,ms);
            }
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
            for (auto mod: modules) tEnvModule_setParameter(mod,EnvSustain,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Release param
    chowdsp::TimeMsParameter::Ptr releaseParam
    {
        juce::ParameterID { "release", 100 },
        "Release",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 20000.0f, 4000.f),
        3.0f,
        all_params[EnvParams::EnvRelease],
        [this] (float val){
            const auto range = chowdsp::ParamUtils::createNormalisableRange(0.0f, 20000.f, 4000.f);
            const float ms = range.convertFrom0to1(val);
            for (auto mod : modules)
            {
                tEnvModule_setParameter(mod, EnvRelease, ms);
            }
        }
    };

    // Leak param
    chowdsp::FloatParameter::Ptr leakParam {
        juce::ParameterID { "leak", 100 },
        "Leak",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.0f,
        all_params[EnvParams::EnvLeak],
        [this] (float val) {
            for (auto mod: modules) tEnvModule_setParameter(mod,EnvLeak,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Shape param
    chowdsp::FloatParameter::Ptr shapeParam {
        juce::ParameterID { "shape", 100 },
        "Shape",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.0f,
        all_params[EnvParams::EnvShape],
        [this] (float val) {
            for (auto mod: modules) tEnvModule_setParameter(mod,EnvShape,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };


};



class EnvModuleProcessor : public ModulatorStateBase<PluginStateImpl_<EnvParamHolder>>
{
public:
    EnvModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree&, LEAF* leaf, juce::UndoManager*);
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

#endif //ELECTROSYNTH_ENVMODULEPROCESSOR_H
