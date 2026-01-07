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
                                        add(inputGain,offset, shape);
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
    chowdsp::MidiHzParameter::Ptr inputGain {
        juce::ParameterID{"inputGain" , 100},
        "Input Gain",
        chowdsp::ParamUtils::createNormalisableRange(0.0f, 5.f, 2.5f),
        1.0f,
        all_params[SoftClipModuleParams::SoftClipInputGain],
        [this](float val)
        {
            for (auto mod: modules)    tSoftClipModule_setParameter(mod,SoftClipInputGain,val);

        DBG("Soft Clip [0 - 1]" + juce::String(val) + " .. .  Soft Clip actual Val" + juce::String(modules[0]->inputGain));
        }
    };

    chowdsp::FloatParameter::Ptr offset {
        juce::ParameterID{"offset", 100},
        "offset",
        chowdsp::ParamUtils::createNormalisableRange(-1.0f, 1.0f, 0.0f),
        0.f,
        all_params[SoftClipModuleParams::SoftClipOffset],
        [this](float val)
        {for (auto mod: modules)                 tSoftClipModule_setParameter(mod,SoftClipOffset,val);

                                           },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
    chowdsp::GainDBParameter::Ptr shape {
        juce::ParameterID{"shape", 100},
        "shape",
        chowdsp::ParamUtils::createNormalisableRange(0.0f, 1.0f, 0.5f),
        0.5f,
        all_params[SoftClipModuleParams::SoftClipShape],
        [this](float val)
        {for (auto mod: modules)    tSoftClipModule_setParameter(mod,SoftClipShape,val);
                                            },
    };

};



class SoftClipModuleProcessor : public ProcessorStateBase<PluginStateImpl_<SoftClipParams>>
{
public:
    SoftClipModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf,juce::UndoManager*);


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
