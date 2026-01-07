//
// Created by Myra Norton on 7/25/25.
//

#ifndef ELECTROSYNTH_DELAYMODULEPROCESSOR_H
#define ELECTROSYNTH_DELAYMODULEPROCESSOR_H
#include "DelayModule.h"
#include "PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "Processors/ProcessorBase.h"

namespace electrosynth{
    namespace utils
    {
        float stringToHarmonicVal(const juce::String &s);
        juce::String harmonicValToString(float harmonic);
    }
}

struct DelayParams : public LEAFParams<tDelayModule >
{
    DelayParams(LEAF* leaf) : LEAFParams<tDelayModule>(leaf)
    {
                                        add(time,feedback, drywet);
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

    // create the 3 parameters of the delay
    chowdsp::TimeMsParameter::Ptr time {
        juce::ParameterID{"time" , 100},
        "Time",
        chowdsp::ParamUtils::createNormalisableRange(0.0f, 5000.f, 1000.0f),
        500.f,
        all_params[DelayModelParams::DelayTime],
        [this](float val)
        {
            for (auto mod: modules) mod->setterFunctions[DelayModelParams::DelayTime](mod,val);
        // DBG("Delay [0 - 1]" + juce::String(val) + " .. .  Delay actual Val" + juce::String(modules[0]->timeKnob));
        }
    };

    chowdsp::PercentParameter::Ptr feedback {
        juce::ParameterID{"feedback", 100},
        "Feedback",
        all_params[DelayModelParams::DelayFeedback],
        [this](float val)
        {for (auto mod: modules) mod->setterFunctions[DelayModelParams::DelayFeedback](mod,val);
                                           }
    };
    chowdsp::PercentParameter::Ptr drywet {
        juce::ParameterID{"drywet", 100},
        "Dry/Wet",
        all_params[DelayModelParams::DelayDryWet],
        [this](float val)
        {for (auto mod: modules) mod->setterFunctions[DelayModelParams::DelayDryWet](mod,val);
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
        return std::make_unique<electrosynth::ParametersView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
};

#endif
