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



    // Attack param
    chowdsp::TimeMsParameter::Ptr attackParam
    {
        juce::ParameterID { "attack", 100 },
            "Attack",
            chowdsp::ParamUtils::createNormalisableRange (0.0f, 10000.0f, 500.0f),
            1.0f,
            &module->params[EnvParams::EnvAttack],
            [this] (float val) {
                module->setterFunctions[EnvParams::EnvAttack](this->module, val);
            }
    };





    // Decay param
    chowdsp::TimeMsParameter::Ptr decayParam {
        juce::ParameterID { "decay", 100 },
        "Decay",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1000.0f, 500.0f),
        0.0f,
        &module->params[EnvParams::EnvDecay],
        [this] (float val) {
            module->setterFunctions[EnvParams::EnvDecay](this->module, val);
        }
    };


    // Sustain param
    chowdsp::FloatParameter::Ptr sustainParam {
        juce::ParameterID { "sustain", 100 },
        "Sustain",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        1.0f,
        &module->params[EnvParams::EnvSustain],
        [this] (float val) {
            module->setterFunctions[EnvParams::EnvSustain](this->module, val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // Release param
    chowdsp::TimeMsParameter::Ptr releaseParam {
        juce::ParameterID { "release", 100 },
        "Release",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 10000.0f, 500.0f),
        50.0f,
        &module->params[EnvParams::EnvRelease],
        [this] (float val) {
            module->setterFunctions[EnvParams::EnvRelease](this->module, val);
        }
    };

};



class EnvModuleProcessor : public ModulatorStateBase<PluginStateImpl_<EnvParamHolder, _tEnvModule >>
{
public:
    EnvModuleProcessor(juce::ValueTree&, LEAF* leaf);
    void process() override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    electrosynth::ParametersView* createEditor() override
    {
        return new electrosynth::ParametersView(state, state.params, vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString());
    }
};

#endif //ELECTROSYNTH_ENVMODULEPROCESSOR_H
