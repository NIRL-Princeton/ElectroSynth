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
    LFOParamHolder(LEAF* leaf) : LEAFParams(leaf)
    {
        add(rateParam, shapeParam, phaseParam, typeParam, syncNoteOnParam);
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
    // rate param
    chowdsp::FreqHzParameter::Ptr rateParam {
        juce::ParameterID { "rate", 100 },
        "Rate",
        chowdsp::ParamUtils::createNormalisableRange (0.0f,30.f,2.f),
        2.0f,
        all_params[LFOParams::LFORateParam],
        [this] (float val) {
            for (auto mod: modules) tLFOModule_setParameter(mod,LFOParams::LFORateParam,val);
        }
    };

    // shape param
    chowdsp::FloatParameter::Ptr shapeParam {
        juce::ParameterID { "shape", 100 },
        "Shape",
        chowdsp::ParamUtils::createNormalisableRange (0.0f,1.f,0.5f),
        0.5f,
        all_params[LFOParams::LFOShapeParam],
        [this] (float val) {
            for (auto mod: modules) tLFOModule_setParameter(mod,LFOParams::LFOShapeParam,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
    // phase param
    chowdsp::FloatParameter::Ptr phaseParam {
        juce::ParameterID { "phase", 100 },
        "Phase",
        chowdsp::ParamUtils::createNormalisableRange (0.0f,1.0f,0.5f),
        0.0f,
        all_params[LFOParams::LFOPhaseParam],
        [this] (float val) {
            for (auto mod: modules) tLFOModule_setParameter(mod,LFOParams::LFOPhaseParam,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
    // Release param
    chowdsp::FloatParameter::Ptr typeParam {
        juce::ParameterID { "oscType", 100 },
        "OscType",
        chowdsp::ParamUtils::createNormalisableRange (0.0f,5.f,2.5f, 1.f),
        0.f,
        all_params[LFOParams::LFOType],
        [this] (float val) {
            for (auto mod: modules) tLFOModule_setParameter(mod,LFOParams::LFOType,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // sync to note on param
    chowdsp::FloatParameter::Ptr syncNoteOnParam {
        juce::ParameterID { "syncToNoteOn", 100 },
        "Sync To Note On",
        chowdsp::ParamUtils::createNormalisableRange (0.0f,1.f,0.5f, 1.f),
        0.f,
        all_params[LFOParams::LFOSyncNoteOnParam],
        [this] (float val) {
            for (auto mod: modules) tLFOModule_setParameter(mod,LFOParams::LFOSyncNoteOnParam,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

};

class LFOModuleProcessor: public ModulatorStateBase<PluginStateImpl_<LFOParamHolder >>
{
public:
    LFOModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree&, LEAF* leaf,juce::UndoManager*);
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override{};
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    std::unique_ptr<SynthSection> createEditor() override
    {
        return std::make_unique<electrosynth::ParametersView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
    void process() override;

};

#endif //ELECTORSYNTH_LFOMODULEPROCESSOR_H
