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

        add(delayTime,delayHP,delayLP,delayFB,delayGain,delayMix );
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
        chowdsp::TimeMsParameter::Ptr delayTime {
        juce::ParameterID { "delayTime", 100 },
        "Time",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 3000.0f, 1500.0f),
        1500.f,
        all_params[DelParams::DelayTime],
        [this](float val)
        {
            for (auto mod: modules)
            {
                tDelayModule_setParameter(mod,DelayTime, val);
            }
        }
    };

    // hp cutoff
    chowdsp::FloatParameter::Ptr delayHP {
        ParameterID{"delayHP", 100},
        "HP Cutoff",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.0f, 0.5f),
        0.f,
        all_params[DelayHPCutoff],
        [this](float val)
        {
            for (auto mod: modules)  tDelayModule_setParameter(mod,DelayHPCutoff,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // hp cutoff
    chowdsp::FloatParameter::Ptr delayLP {
        juce::ParameterID{"delayLP", 100},
        "LP Cutoff",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.0f, 0.5f),
        1.0f,
        all_params[DelayLPCutoff],
        [this](float val)
        {
            for (auto mod: modules)  tDelayModule_setParameter(mod,DelayLPCutoff,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // hp cutoff
    chowdsp::FloatParameter::Ptr delayFB {
        juce::ParameterID{"delayFB", 100},
        "Feedback",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.0f, 0.7f),
        1.0f,
        all_params[DelayFeedback],
        [this](float val)
        {
            for (auto mod: modules)  tDelayModule_setParameter(mod,DelayFeedback,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };


    // gain
    chowdsp::GainDBParameter::Ptr delayGain {
        juce::ParameterID{"delayGain", 100},
        "Gain",
        chowdsp::ParamUtils::createNormalisableRange(0.0f, 2.0f, 1.0f),
        1.f,
        all_params[DelParams::DelayGain],
        [this](float val)
        {for (auto mod: modules)    tDelayModule_setParameter(mod,DelayGain,val);
        },
    };



    chowdsp::FloatParameter::Ptr delayMix {
        juce::ParameterID{"delayMix", 100},
        "Mix",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.0f, 0.5f),
        1.0f,
        all_params[DelayMix],
        [this](float val)
        {
            for (auto mod: modules)  tDelayModule_setParameter(mod,DelayMix,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
};

class DelayModuleProcessor : public ProcessorStateBase<PluginStateImpl_<DelayParams>>
{
public:
    DelayModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf,juce::UndoManager*);
    electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept override {
        return electrosynth::audio::makeProcessorDescriptor();
    }
    void process() override;
    std::unique_ptr<SynthSection> createEditor() override
    {
        return std::make_unique<electrosynth::FxModuleTemplateView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {}
    void releaseResources() override {}
};

#endif // ELECTORSYNTH_DELAYMODULEPROCESSOR_H
