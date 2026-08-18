//
// Created by Jeff Snyder on 1/7/26.
//

#ifndef SOFTCLIPMODULEPROCESSOR_H
#define SOFTCLIPMODULEPROCESSOR_H
#include "SoftClipModule.h"
#include "PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "Processors/ProcessorBase.h"


struct SoftClipParams : public LEAFParams<_tSoftClipModule >
{
    SoftClipParams(LEAF* leaf) : LEAFParams<_tSoftClipModule>(leaf)
    {
        add(inputGain,offset, shape, outputGain, mix);
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
    chowdsp::GainDBParameter::Ptr inputGain {
        juce::ParameterID{"inputGain" , 100},
        "Input Gain",
        chowdsp::ParamUtils::createNormalisableRange(-80.0f, 10.f, 0.f),
        0.0f,
        all_params[SoftClipModuleParams::SoftClipInputGain],
        [this](float val)
        {
            for (auto mod: modules)    tSoftClipModule_setParameter(mod,SoftClipInputGain,val);

        //DBG("Soft Clip [0 - 1]" + juce::String(val) + " .. .  Soft Clip actual Val" + juce::String(modules[0]->inputGain));
        }
    };

    chowdsp::FloatParameter::Ptr offset {
        juce::ParameterID{"offset", 100},
        "Offset",
        chowdsp::ParamUtils::createNormalisableRange(-1.0f, 1.0f, 0.0f),
        0.f,
        all_params[SoftClipModuleParams::SoftClipOffset],
        [this](float val)
        {for (auto mod: modules)
            tSoftClipModule_setParameter(mod,SoftClipOffset,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
    chowdsp::FloatParameter::Ptr shape {
        juce::ParameterID{"shape", 100},
        "Shape",
        chowdsp::ParamUtils::createNormalisableRange(0.0f, 1.0f, 0.5f),
        0.5f,
        all_params[SoftClipModuleParams::SoftClipShape],
        [this](float val)
        {for (auto mod: modules)    tSoftClipModule_setParameter(mod,SoftClipShape,val);
                                            },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
    chowdsp::GainDBParameter::Ptr outputGain {
        juce::ParameterID{"outputGain", 100},
        "Output Gain",
        chowdsp::ParamUtils::createNormalisableRange(-80.0f, 10.0f, 0.f),
        0.0f,
        all_params[SoftClipModuleParams::SoftClipOutputGain],
        [this](float val) {
            for (auto mod: modules)    tSoftClipModule_setParameter(mod,SoftClipOutputGain,val);
        },
    };
    chowdsp::PercentParameter::Ptr mix {
        juce::ParameterID{"mix", 100},
        "Mix",
        all_params[SoftClipModuleParams::SoftClipMix],
        [this](float val)
        {for (auto mod: modules)    tSoftClipModule_setParameter(mod,SoftClipMix,val);
        },
        1.f,
        false
};

};



class SoftClipModuleProcessor : public ProcessorStateBase<PluginStateImpl_<SoftClipParams>>
{
public:
    SoftClipModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf,juce::UndoManager*);

    electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept override {
        return electrosynth::audio::makeProcessorDescriptor();
    }


    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
    void releaseResources() override {}
    std::unique_ptr<SynthSection> createEditor() override
    {
        return std::make_unique<electrosynth::ParametersView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
};

#endif //SOFTCLIPMODULEPROCESSOR_H
