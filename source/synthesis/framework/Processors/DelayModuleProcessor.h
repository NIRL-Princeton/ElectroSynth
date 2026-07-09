//
// Created by Myra Norton on 1/22/26.
//

#ifndef ELECTORSYNTH_DELAYMODULEPROCESSOR_H
#define ELECTORSYNTH_DELAYMODULEPROCESSOR_H

#include "DelayModule.h"
#include "PluginStateImpl_.h"
#include "ParameterView/FxModuleTemplateView.h"
#include "Identifiers.h"
#include "Processors/ProcessorBase.h"

struct DelayParams : public LEAFParams<_tDelayModule >
{
    DelayParams(LEAF* leaf) : LEAFParams<_tDelayModule>(leaf)
    {
        add(time,gain);
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

    // time
        chowdsp::TimeMsParameter::Ptr time {
        juce::ParameterID { "delayTime", 100 },
        "Time",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1000.0f, 500.0f),
        0.0f,
        all_params[DelParams::DelayTime],
        [this](float val)
        {for (auto mod: modules)    tDelayModule_setParameter(mod,DelayTime,val);
        }
    };

    // gain
    chowdsp::GainDBParameter::Ptr gain {
        juce::ParameterID{"gain", 100},
        "Gain",
        chowdsp::ParamUtils::createNormalisableRange(0.0f, 2.0f, 1.0f),
        1.f,
        all_params[DelParams::DelayGain],
        [this](float val)
        {for (auto mod: modules)    tDelayModule_setParameter(mod,DelayGain,val);
        },
    };
};

class DelayModuleProcessor : public ProcessorStateBase<PluginStateImpl_<DelayParams>>
{
public:
    DelayModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf,juce::UndoManager*);

    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
    void releaseResources() override {}
    std::unique_ptr<SynthSection> createEditor() override
    {
        return std::make_unique<electrosynth::FxModuleTemplateView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
};

#endif // ELECTORSYNTH_DELAYMODULEPROCESSOR_H
