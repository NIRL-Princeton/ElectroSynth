//
// Created by Davis Polito on 1/27/25.
//

#ifndef ELECTORSYNTH_LFOMODULEPROCESSOR_H
#define ELECTORSYNTH_LFOMODULEPROCESSOR_H
#include "PluginStateImpl_.h"
#include "Identifiers.h"
#include "ModulatorBase.h"

struct LFOParamHolder : public LEAFParams<_tLFOModule>
{
    LFOParamHolder(LEAF* leaf) : LEAFParams<_tLFOModule>(leaf)
    {
        add(rateParam);
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
    chowdsp::FreqHzParameter::Ptr rateParam {
        juce::ParameterID { "rate", 100 },
        "Rate",
        chowdsp::ParamUtils::createNormalisableRange (0.0f,30.f,15.f),
        15.0f,
        all_params[LFOParams::LFORateParam],
        [this] (float val) {
            for (auto mod: modules) mod->setterFunctions[LFOParams::LFORateParam](mod, val);
        }
    };

    // Release param
    chowdsp::FloatParameter::Ptr shapeParam {
        juce::ParameterID { "shape", 100 },
        "Shape",
        chowdsp::ParamUtils::createNormalisableRange (0.0f,1.f,0.5f),
        0.5f,
        all_params[LFOParams::LFOShapeParam],
        [this] (float val) {
            for (auto mod: modules) mod->setterFunctions[LFOParams::LFOShapeParam](mod, val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
    // Release param
    chowdsp::FloatParameter::Ptr phaseParam {
        juce::ParameterID { "phase", 100 },
        "Phase",
        chowdsp::ParamUtils::createNormalisableRange (0.0f,1.0f,0.5f),
        0.0f,
        all_params[LFOParams::LFOShapeParam],
        [this] (float val) {
            for (auto mod: modules) mod->setterFunctions[LFOParams::LFOShapeParam](mod, val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

};

class LFOModuleProcessor: public ModulatorStateBase<PluginStateImpl_<LFOParamHolder, _tLFOModule >>
{
public:
    LFOModuleProcessor(juce::ValueTree&, LEAF* leaf);
    void process() override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    electrosynth::ParametersView* createEditor() override
    {
        return new electrosynth::ParametersView(state, state.params, vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString());
    }

};

#endif //ELECTORSYNTH_LFOMODULEPROCESSOR_H
