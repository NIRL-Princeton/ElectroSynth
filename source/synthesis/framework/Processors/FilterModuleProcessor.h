// Created by Davis Polito on 8/8/24.
//

#ifndef ELECTROSYNTH_FILTERMODULEPROCESSOR_H
#define ELECTROSYNTH_FILTERMODULEPROCESSOR_H
#include "FilterModule.h"
#include "PluginStateImpl_.h"
#include "ParameterView/FxModuleTemplateView.h"
#include "Identifiers.h"
#include "Processors/ProcessorBase.h"

// FilterModuleProcessor.h creates a struct of type <_tFiltModule>, which is defined in FilterModule.h,
// to represent a filter module, and defines each knob

namespace electrosynth{
    namespace utils
    {
        float stringToHarmonicVal(const juce::String &s);
        juce::String harmonicValToString(float harmonic);
    }
}

// creating the parameters associated with a filter module [cutoff, Q, and amp]
struct FilterParams : public LEAFParams<_tFiltModule > {
    FilterParams(LEAF* leaf) : LEAFParams<_tFiltModule>(leaf) {
        add(cutoff,Q, amp);
    }
    //add env watch param so that it isn't null
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

    // this is where the knob labeled "Cutoff" is created
    chowdsp::MidiHzParameter::Ptr cutoff {
        juce::ParameterID{"cutoff" , 100},
        "Cutoff",
        chowdsp::ParamUtils::createNormalisableRange(0.0f, 127.f, 60.f),
        60.f,
        all_params[FiltParams::FiltCutoff],
        [this](float val)
        {
            for (auto mod: modules) tFiltModule_setParameter(mod,FiltCutoff,val);
            DBG("Filt [0 - 1]" + juce::String(val) + " .. .  Filt actual Val" + juce::String(modules[0]->cutoffKnob));
        }
    };

    // this is where the knob labeled "Q" is created
    chowdsp::FloatParameter::Ptr Q {
        juce::ParameterID{"resonance", 100},
        "Q",
        chowdsp::ParamUtils::createNormalisableRange(0.1f, 1.0f, 0.5f),
        1.f,
        all_params[FiltParams::FiltResonance],
        [this](float val)
        {
            for (auto mod: modules)  tFiltModule_setParameter(mod,FiltResonance,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // this is where the knob labeled "Amp" is created
    chowdsp::GainDBParameter::Ptr amp {
        juce::ParameterID{"amp", 100},
        "Amp",
        chowdsp::ParamUtils::createNormalisableRange(0.0f, 2.0f, 1.0f),
        1.f,
        all_params[FiltParams::FiltGain],
        [this](float val)
        {
            for (auto mod: modules) tFiltModule_setParameter(mod,FiltGain,val);
        },
    };
};

class FilterModuleProcessor : public ProcessorStateBase<PluginStateImpl_<FilterParams>> {
public:
    FilterModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf,juce::UndoManager*);
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
    void releaseResources() override {}
    std::unique_ptr<SynthSection> createEditor() override {
        auto name = state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString();
        // Filter can live in either lane: horizontal ParametersView as a sound
        // module, vertical FxModuleTemplateView as an effect module (FX panel).
        if (state.hasType(IDs::SOUNDMODULE))
            return std::make_unique<electrosynth::ParametersView>(state_, state_.params, name);
        return std::make_unique<electrosynth::FxModuleTemplateView>(state_, state_.params, name);
    }
};

#endif //ELECTROSYNTH_OSCILLATORMODULEPROCESSOR_H
