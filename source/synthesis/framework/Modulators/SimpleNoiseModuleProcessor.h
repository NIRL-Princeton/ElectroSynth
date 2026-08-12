//
// Created by Matthew McWeeney on 7/14/26.
//

#ifndef ELECTORSYNTH_SIMPLENOISEMODULEPROCESSOR_H
#define ELECTORSYNTH_SIMPLENOISEMODULEPROCESSOR_H
#include "SimpleNoiseModule.h"
#include "../PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "ModulatorBase.h"

struct SimpNoiseParamHolder : public LEAFParams<_tSimpNoiseModule>
{
    SimpNoiseParamHolder(LEAF* leaf) : LEAFParams(leaf)
    {
        add(amp);
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
    // Release param
    chowdsp::FloatParameter::Ptr amp {
        juce::ParameterID { "amp", 100 },
        "Amp",
        chowdsp::ParamUtils::createNormalisableRange (0.0f,1.f,.5f),
        1.0f,
        all_params[SimpNosParams::SimpNoiseAmp],
        [this] (float val) {
            for (auto mod: modules) tSimpNoiseModule_setParameter(mod,SimpNosParams::SimpNoiseAmp,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

};

class SimpNoiseModuleProcessor: public ModulatorStateBase<PluginStateImpl_<SimpNoiseParamHolder >>
{
public:
    SimpNoiseModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree&, LEAF* leaf,juce::UndoManager*);
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override{};
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    std::unique_ptr<SynthSection> createEditor() override
    {
        return std::make_unique<electrosynth::ParametersView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
    void process() override;

};

#endif // ELECTORSYNTH_SIMPLENOISEMODULEPROCESSOR_H
