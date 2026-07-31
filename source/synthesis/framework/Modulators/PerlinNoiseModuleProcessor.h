//
// Created by Matthew McWeeney on 7/15/26.
//

#ifndef ELECTORSYNTH_PERLINNOISEMODULEPROCESSOR_H
#define ELECTORSYNTH_PERLINNOISEMODULEPROCESSOR_H
#include "PerlinNoiseModule.h"
#include "../PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "ModulatorBase.h"

struct PerlNoiseParamHolder : public LEAFParams<_tPerlNoiseModule>
{
    PerlNoiseParamHolder(LEAF* leaf) : LEAFParams(leaf)
    {
        add(gain, rateMs, energy);
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

    chowdsp::GainDBParameter::Ptr gain {
        juce::ParameterID { "gain", 100 },
        "Gain",
        chowdsp::ParamUtils::createNormalisableRange (0.0f,2.f,1.f),
        1.0f,
        all_params[PerlNosParams::PerlNoiseGain],
        [this] (float val) {
            for (auto mod: modules) tPerlNoiseModule_setParameter(mod,PerlNosParams::PerlNoiseGain,val);
        }
    };

    chowdsp::FloatParameter::Ptr rateMs {
        juce::ParameterID { "rateMs", 100 },
        "RateMs",
        chowdsp::ParamUtils::createNormalisableRange (0.1f,2000.f,100.f),
        100.0f,
        all_params[PerlNosParams::PerlNoiseRate],
        [this] (float val) {
            for (auto mod: modules) tPerlNoiseModule_setParameter(mod,PerlNosParams::PerlNoiseRate,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr energy {
        juce::ParameterID { "energy", 100 },
        "Energy",
        chowdsp::ParamUtils::createNormalisableRange (0.f,1.f,.5f),
        0.5f,
        all_params[PerlNosParams::PerlNoiseEnergy],
        [this] (float val) {
            for (auto mod: modules) tPerlNoiseModule_setParameter(mod,PerlNosParams::PerlNoiseEnergy,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

};

class PerlNoiseModuleProcessor: public ModulatorStateBase<PluginStateImpl_<PerlNoiseParamHolder >>
{
public:
    PerlNoiseModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree&, LEAF* leaf,juce::UndoManager*);
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override{};
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    std::unique_ptr<SynthSection> createEditor() override
    {
        return std::make_unique<electrosynth::ParametersView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
    void process() override;

};

#endif // ELECTORSYNTH_PERLINNOISEMODULEPROCESSOR_H
