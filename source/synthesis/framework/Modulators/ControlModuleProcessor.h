//
// Created by Jeff Snyder on 11/15/26.
//

#ifndef CONTROLMODULEPROCESSOR_H
#define CONTROLMODULEPROCESSOR_H


#include "ModulatorBase.h"
//
// Created by Davis Polito on 11/19/24.
//
#include "PluginStateImpl_.h"
#include "Identifiers.h"

struct ControlParamHolder : public LEAFParams<_tControlModule>
{
    ControlParamHolder(LEAF* leaf) : LEAFParams<_tControlModule>(leaf)
    {
        add(ccParam,
            ccVal
            );
    }

    chowdsp::FloatParameter::Ptr controlwatchparam {
        juce::ParameterID { "watch", 100 },
        "watch",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        1.0f,
        all_params[ControlEventWatchFlag],
        [this] (float val) {
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    //ccParam Number
    chowdsp::FloatParameter::Ptr ccParam {
        juce::ParameterID { "ccParam", 100 },
        "Which CC number controls this modulation source",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 16.0f, 8.0f, 1.0f),
        0.0f,
        all_params[ControlParams::ControlCCParam],
        [this] (float val) {
            for (auto mod: modules) tControlModule_setParameter(mod, ControlCCParam, val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // ccValue param
    //ccParam Number
    chowdsp::FloatParameter::Ptr ccVal {
        juce::ParameterID { "ccValue", 100 },
        "Control Value",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.0f,
        all_params[ControlParams::ControlCCValue],
        [this] (float val) {
            for (auto mod: modules) tControlModule_setParameter(mod, ControlCCValue, val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };


};



class ControlModuleProcessor : public ModulatorStateBase<PluginStateImpl_<ControlParamHolder>>
{
public:
    ControlModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree&, LEAF* leaf, juce::UndoManager*);
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


#endif //CONTROLMODULEPROCESSOR_H
