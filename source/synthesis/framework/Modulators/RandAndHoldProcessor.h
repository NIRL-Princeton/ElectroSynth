//
// Created by MrKahoobadoo on 9/24/26.
//

#ifndef ELECTORSYNTH_RANDANDHOLDPROCESSOR_H
#define ELECTORSYNTH_RANDANDHOLDPROCESSOR_H

#include "RandAndHoldModule.h"
#include "../PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "ModulatorBase.h"
#include "../Processors/ProcessorBase.h"
#include "ParameterView/FxModuleTemplateView.h"
#include "PluginStateImpl_.h"

struct RandHoldParamHolder : public LEAFParams<_tRandAndHoldModule>
{
    RandHoldParamHolder(LEAF* leaf) : LEAFParams(leaf)
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

    // chowdsp::FloatParameter::Ptr threshold {
    //     juce::ParameterID { "threshold", 100 },
    //     "Threshold",
    //     chowdsp::ParamUtils::createNormalisableRange (0.f,2.f,1.f),
    //     1.0f,
    //     all_params[RandHoldParams::RandHoldThreshold],
    //     [this] (float val) {
    //         for (auto mod: modules) tRandAndHoldModule_setParameter(mod,RandHoldParams::RandHoldThreshold,val);
    //     },
    //     &chowdsp::ParamUtils::floatValToString,
    //     &chowdsp::ParamUtils::stringToFloatVal
    // };

    // chowdsp::FloatParameter::Ptr triggerToggle {
    //     juce::ParameterID { "triggerToggle", 100 },
    //     "TriggerToggle",
    //     chowdsp::ParamUtils::createNormalisableRange (0.f,1.0f,.5f, 1.f),
    //     0.f,
    //     all_params[RandHoldParams::RandHoldTriggerToggle],
    //     [this] (float val) {
    //         for (auto mod: modules) tRandAndHoldModule_setParameter(mod,RandHoldParams::RandHoldTriggerToggle,val);
    //     },
    //     &chowdsp::ParamUtils::floatValToString,
    //     &chowdsp::ParamUtils::stringToFloatVal
    // };

    chowdsp::FloatParameter::Ptr amp {
        juce::ParameterID { "amp", 100 },
        "Amp",
        chowdsp::ParamUtils::createNormalisableRange (0.f,2.f,1.f),
        1.0f,
        all_params[RandHoldParams::RandHoldAmp],
        [this] (float val) {
            for (auto mod: modules) tRandAndHoldModule_setParameter(mod,RandHoldParams::RandHoldAmp,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

};

// class RandAndHoldProcessor : public ProcessorStateBase<PluginStateImpl_<RandHoldParamHolder>>
// {
// public:
//     RandAndHoldProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf,juce::UndoManager*);
//     electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept override {
//         return electrosynth::audio::makeProcessorDescriptor();
//     }
//     void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
//     void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
//     void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
//     void releaseResources() override {}
//     std::unique_ptr<SynthSection> createEditor() override {
//         auto name = state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString();
//         // module, vertical FxModuleTemplateView as an effect module (FX panel).
//         if (state.hasType(IDs::SOUNDMODULE))
//             return std::make_unique<electrosynth::ParametersView>(state_, state_.params, name);
//         //return std::make_unique<electrosynth::FxModuleTemplateView>(state_, state_.params, name);
//     }
// };

class RandAndHoldProcessor: public ModulatorStateBase<PluginStateImpl_<RandHoldParamHolder >>
{
public:
    RandAndHoldProcessor(electrosynth::SoundEngine* engine,juce::ValueTree&, LEAF* leaf,juce::UndoManager*);
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override{};
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
    std::unique_ptr<SynthSection> createEditor() override
    {
        return std::make_unique<electrosynth::ParametersView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
    void process() override;

};

// class RandAndHoldProcessor : public ModulatorStateBase<PluginStateImpl_<RandHoldParamHolder>>
// {
// public:
//     RandAndHoldProcessor(electrosynth::SoundEngine* engine, juce::ValueTree&, LEAF* leaf,juce::UndoManager*);
//     electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept override {
//         return electrosynth::audio::makeProcessorDescriptor();
//     }
//     void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
//     void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&);
//     void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
//     void releaseResources() override {}
//     std::unique_ptr<SynthSection> createEditor() override
//     {
//         return std::make_unique<electrosynth::ParametersView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
//     }
// };

#endif // ELECTORSYNTH_RANDANDHOLDPROCESSOR_H
