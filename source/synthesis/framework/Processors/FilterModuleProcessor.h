// Created by Davis Polito on 8/8/24.
//

#ifndef ELECTROSYNTH_FILTERMODULEPROCESSOR_H
#define ELECTROSYNTH_FILTERMODULEPROCESSOR_H
#include "FilterModule.h"
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



struct FilterParams : public LEAFParams<_tFiltModule >
{
    FilterParams(LEAF* leaf) : LEAFParams<_tFiltModule>(leaf)
    {
                                        add(cutoff,Q, amp);
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
    chowdsp::MidiHzParameter::Ptr cutoff {
        juce::ParameterID{"cutoff" , 100},
        "Cutoff",
        chowdsp::ParamUtils::createNormalisableRange(0.0f, 127.f, 60.f),
        60.f,
        all_params[FiltParams::FiltCutoff],
        [this](float val)
        {
            for (auto mod: modules) mod->setterFunctions[FiltParams::FiltCutoff](mod,val);
        DBG("Filt [0 - 1]" + juce::String(val) + " .. .  Filt actual Val" + juce::String(modules[0]->cutoffKnob));
        }
    };

    chowdsp::FloatParameter::Ptr Q {
        juce::ParameterID{"resonance", 100},
        "Q",
        chowdsp::ParamUtils::createNormalisableRange(0.1f, 1.0f, 0.5f),
        1.f,
        all_params[FiltParams::FiltResonance],
        [this](float val)
        {for (auto mod: modules) mod->setterFunctions[FiltParams::FiltResonance](mod,val);
                                           },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
    chowdsp::GainDBParameter::Ptr amp {
        juce::ParameterID{"amp", 100},
        "amp",
        chowdsp::ParamUtils::createNormalisableRange(0.0f, 2.0f, 1.0f),
        1.f,
        all_params[FiltParams::FiltGain],
        [this](float val)
        {for (auto mod: modules) mod->setterFunctions[FiltParams::FiltGain](mod,val);
                                            },
    };

};



class FilterModuleProcessor : public ProcessorStateBase<PluginStateImpl_<FilterParams>>
{
public:
    FilterModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf);


    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
    void releaseResources() override {}
    electrosynth::ParametersView* createEditor() override
    {
        return new electrosynth::ParametersView(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
};

#endif //ELECTROSYNTH_OSCILLATORMODULEPROCESSOR_H
