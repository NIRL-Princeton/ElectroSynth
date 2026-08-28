//
// Created by Matthew McWeeney on 8/28/26.
//

#ifndef ELECTORSYNTH_SAMPLEANDHOLDPROCESSOR_H
#define ELECTORSYNTH_SAMPLEANDHOLDPROCESSOR_H

#include "SampleAndHoldModule.h"
#include "../PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "ModulatorBase.h"
#include "../Processors/ProcessorBase.h"
#include "ParameterView/FxModuleTemplateView.h"
#include "PluginStateImpl_.h"

struct SampHoldParamHolder : public LEAFParams<_tSampleAndHoldModule>
{
    SampHoldParamHolder(LEAF* leaf) : LEAFParams(leaf)
    {
        add(threshold, frequency);
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

    chowdsp::GainDBParameter::Ptr threshold {
        juce::ParameterID { "threshold", 100 },
        "Threshold",
        chowdsp::ParamUtils::createNormalisableRange (0.f,4.f,1.f),
        1.0f,
        all_params[SampHoldParams::SampHoldThreshold],
        [this] (float val) {
            for (auto mod: modules) tSampleAndHoldModule_setParameter(mod,SampHoldParams::SampHoldThreshold,val);
        }
    };

    chowdsp::FreqHzParameter::Ptr frequency {
        juce::ParameterID { "frequency", 100 },
        "Frequency",
        chowdsp::ParamUtils::createNormalisableRange (0.f,20000.0f,4000.f),
        2.f,
        all_params[SampHoldParams::SampHoldFrequency],
        [this] (float val) {
            for (auto mod: modules) tSampleAndHoldModule_setParameter(mod,SampHoldParams::SampHoldFrequency,val);
        }
    };
};

// class SampleAndHoldProcessor: public ModulatorStateBase<PluginStateImpl_<SampHoldParamHolder >>
// {
// public:
//     SampleAndHoldProcessor(electrosynth::SoundEngine* engine,juce::ValueTree&, LEAF* leaf,juce::UndoManager*);
//     void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override{};
//     void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
//     void releaseResources() override {}
//     std::unique_ptr<SynthSection> createEditor() override
//     {
//         return std::make_unique<electrosynth::ParametersView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
//     }
//     void process() override;
//
// };

class SampleAndHoldProcessor : public ProcessorStateBase<PluginStateImpl_<SampHoldParamHolder>>
{
public:
    SampleAndHoldProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf,juce::UndoManager*);
    electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept override {
        return electrosynth::audio::makeProcessorDescriptor();
    }
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
    void releaseResources() override {}
    std::unique_ptr<SynthSection> createEditor() override {
        auto name = state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString();
        // module, vertical FxModuleTemplateView as an effect module (FX panel).
        if (state.hasType(IDs::SOUNDMODULE))
            return std::make_unique<electrosynth::ParametersView>(state_, state_.params, name);
        return std::make_unique<electrosynth::FxModuleTemplateView>(state_, state_.params, name);
    }
};



#endif // ELECTORSYNTH_SAMPLEANDHOLDPROCESSOR_H
