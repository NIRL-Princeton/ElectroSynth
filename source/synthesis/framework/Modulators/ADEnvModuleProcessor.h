//
// Created by Matthew McWeeney on 9/6/26.
//

#ifndef ELECTORSYNTH_ADENVMODULEPROCESSOR_H
#define ELECTORSYNTH_ADENVMODULEPROCESSOR_H

#include "ModulatorBase.h"
#include "PluginStateImpl_.h"
#include "Identifiers.h"
#include "ADEnvModule.h"

struct ADEnvParamHolder : public LEAFParams<_tADEnvModule>
{
    ADEnvParamHolder(LEAF* leaf) : LEAFParams<_tADEnvModule>(leaf)
    {
        add(velocityParam,
            attackParam,
            decayParam
            );
    }

    chowdsp::FloatParameter::Ptr envwatchparam {
        juce::ParameterID { "watch", 100 },
        "watch",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        1.0f,
        all_params[ADEnvEventWatchFlag],
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
        all_params[ADEnvVelocitySense],
        [this] (float val) {
            for (auto mod: modules) tADEnvModule_setParameter(mod, ADEnvVelocitySense, val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Attack param
    chowdsp::TimeMsParameter::Ptr attackParam
    {
        juce::ParameterID { "attack", 100 },
            "Attack",
            chowdsp::ParamUtils::createNormalisableRange (0.005f, 20000.0f, 4000.f),
            0.005f,
            all_params[ADEnvAttack],
            [this] (float val) {
                for (auto mod: modules) tADEnvModule_setParameter(mod,ADEnvAttack,val);
            }
    };



    // Decay param
    chowdsp::TimeMsParameter::Ptr decayParam {
        juce::ParameterID { "decay", 100 },
        "Decay",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 20000.0f, 4000.f),
        0.3f,
        all_params[ADEnvDecay],
        [this] (float val) {
            for (auto mod: modules)tADEnvModule_setParameter(mod,ADEnvDecay,val);
        }
    };
};



class ADEnvModuleProcessor : public ModulatorStateBase<PluginStateImpl_<ADEnvParamHolder>>
{
public:
    ADEnvModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree&, LEAF* leaf, juce::UndoManager*);
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

#endif // ELECTORSYNTH_ADENVMODULEPROCESSOR_H
