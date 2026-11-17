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



// creating the parameters associated with a VCA
struct VCAParamHolder : public LEAFParams<_tVCAModule > {
    VCAParamHolder(LEAF* leaf) : LEAFParams<_tVCAModule>(leaf) {

        add(CVOffsetParam,ampParam,gainParam);

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


    chowdsp::FloatParameter::Ptr CVOffsetParam {
        juce::ParameterID { "CVOffset", 100 },
        "CV Offset",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.0f,
        all_params[VCAParams::VCAOffset],
        [this] (float val) {
            for (auto mod: modules)
                tVCAModule_setParameter(mod, VCAOffset, val);

        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
    chowdsp::FloatParameter::Ptr ampParam {
        juce::ParameterID { "amp", 100 },
        "Amp (Linear)",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        1.0f,
        all_params[VCAParams::VCAAmp],
        [this] (float val) {
            for (auto mod: modules)
                tVCAModule_setParameter(mod, VCAAmp, val);

        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
    chowdsp::GainDBParameter::Ptr gainParam{
        juce::ParameterID{"gain", 100},
        "Gain (Exp)",
        chowdsp::ParamUtils::createNormalisableRange (-10000.f,12.f,0.f),
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
    void process () override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
    void releaseResources() override {}
    std::unique_ptr<SynthSection> createEditor() override {
        auto name = state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString();
        return std::make_unique<electrosynth::FxModuleTemplateView>(state_, state_.params, name);
    }
};

#endif // ELECTORSYNTH_VCAMODULEPROCESSOR_H
