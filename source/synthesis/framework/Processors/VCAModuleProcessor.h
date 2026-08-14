//
// Created by Matthew McWeeney on 8/14/26.
//

#ifndef ELECTORSYNTH_VCAMODULEPROCESSOR_H
#define ELECTORSYNTH_VCAMODULEPROCESSOR_H

#include "VCAModule.h"
#include "PluginStateImpl_.h"
#include "ParameterView/FxModuleTemplateView.h"
#include "Identifiers.h"
#include "Processors/ProcessorBase.h"

namespace electrosynth{
    namespace utils
    {
        float stringToHarmonicVal(const juce::String &s);
        juce::String harmonicValToString(float harmonic);
    }
}

// creating the parameters associated with a filter module [cutoff, Q, and amp]
struct VCAParamHolder : public LEAFParams<_tVCAModule > {
    VCAParamHolder(LEAF* leaf) : LEAFParams<_tVCAModule>(leaf) {
        add(gainParam);
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

    chowdsp::GainDBParameter::Ptr gainParam{
        juce::ParameterID{"gain", 100},
        "Gain",
        chowdsp::ParamUtils::createNormalisableRange (-80.f,10.f,0.f),
        0.f,
        all_params[VCAParams::VCAGain],
        [this](float val) {
            for (auto mod: modules)
                tVCAModule_setParameter(mod, VCAGain, val);
        }
    };
};


class VCAModuleProcessor : public ProcessorStateBase<PluginStateImpl_<VCAParamHolder>> {
public:
    VCAModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf,juce::UndoManager*);
    electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept override {
        return electrosynth::audio::makeProcessorDescriptor();
    }
    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
    void releaseResources() override {}
    std::unique_ptr<SynthSection> createEditor() override {
        auto name = state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString();
        return std::make_unique<electrosynth::FxModuleTemplateView>(state_, state_.params, name);
    }
};

#endif // ELECTORSYNTH_VCAMODULEPROCESSOR_H
