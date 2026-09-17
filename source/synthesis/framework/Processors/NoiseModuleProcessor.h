//
// Created by Matthew McWeeney on 7/9/26.
//

#ifndef ELECTORSYNTH_NOISEMODULEPROCESSOR_H
#define ELECTORSYNTH_NOISEMODULEPROCESSOR_H
#include "NoiseModule.h"
#include "../PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "ProcessorBase.h"

struct NoiseParams : public LEAFParams<_tNoiseModule>
{
    NoiseParams(LEAF* leaf) : LEAFParams<_tNoiseModule>(leaf)
    {
        add(tilt, peakGain, peakBandwidth, freqKnob, keyFollow, glide, portaType, gain);
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

    // gain
    chowdsp::GainDBParameter::Ptr gain {
        juce::ParameterID{"gain", 100},
        "Gain",
        chowdsp::ParamUtils::createNormalisableRange(-10000.f, 12.f, 0.f),
        0.f,
        all_params[NosParams::NoiseGain],
        [this](float val)
        {for (auto mod: modules)    tNoiseModule_setParameter(mod,NoiseGain,val);
        },
    };

    // tilt
    chowdsp::FloatParameter::Ptr tilt {
        juce::ParameterID { "tilt", 100 },
        "Tilt",
        chowdsp::ParamUtils::createNormalisableRange (0.f, 1.0f, .5f),
        0.5f,
        all_params[NosParams::NoiseTilt],
        [this] (float val)
        {for (auto mod: modules) tNoiseModule_setParameter(mod, NoiseTilt,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // peakGain
    chowdsp::FloatParameter::Ptr peakGain {
        juce::ParameterID{"peakGain", 100},
        "Peak Gain",
        chowdsp::ParamUtils::createNormalisableRange(1.f, 199.f, 100.f),
        1.f,
        all_params[NosParams::NoisePeakGain],
        [this](float val)
        {for (auto mod: modules)    tNoiseModule_setParameter(mod,NoisePeakGain,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    //
    chowdsp::FreqHzParameter::Ptr freqKnob {
        juce::ParameterID{"peakFreq" , 100},
        "Peak Freq",
        chowdsp::ParamUtils::createNormalisableRange(0.1f, 20000.f, 1000.f),
        1000.f,
        all_params[NosParams::NoiseFreqKnob],
        [this](float val)
        {
            for (auto mod: modules) tNoiseModule_setParameter(mod,NoiseFreqKnob,val);
            //DBG("Noise [0 - 1]" + juce::String(val) + " .. .  peakFreq actual Val" + juce::String(modules[0]->peakFreq));
        }
    };

    chowdsp::FloatParameter::Ptr peakBandwidth {
        juce::ParameterID { "peakBandwidth", 100 },
        "Peak Bandwidth",
        chowdsp::ParamUtils::createNormalisableRange (0.0f, 1.0f, 0.5f),
        0.5f,
        all_params[NosParams::NoisePeakBandwidth],
        [this] (float val) {
            for (auto mod: modules)
            {
                tNoiseModule_setParameter(mod,NoisePeakBandwidth,val);
                //tNoiseModule_setParameter(mod, NoiseFreqKnob, *mod->header.params[NoiseFreqKnob]);
            }
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    // keyFollow
    chowdsp::FloatParameter::Ptr keyFollow {
        juce::ParameterID { "keyFollow", 100 },
        "Keyfollow",
        chowdsp::ParamUtils::createNormalisableRange (0.f, 1.0f, .5f),
        0.f,
        all_params[NosParams::NoiseKeyFollow],
        [this] (float val)
        {for (auto mod: modules) tNoiseModule_setParameter(mod, NoiseKeyFollow,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::TimeMsParameter::Ptr glide
    {
        juce::ParameterID{"glide" , 100},
        "Porta",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 8000.f,500.f),
        0.0f,
        all_params[NosParams::NoiseGlide],
        [this]( float val)
        {   float realVal = glide->getCurrentValue();
            for (auto mod : modules)
                tNoiseModule_setParameter(mod,NoiseGlide, realVal);
            //DBG("freq [0 - 1] " + juce::String(val) + " .... glide actual Val" + juce::String(mod->glide));
        }
    };

    // portaType
    chowdsp::FloatParameter::Ptr portaType {
        juce::ParameterID { "portaType", 100 },
        "Porta Type",
        chowdsp::ParamUtils::createNormalisableRange (0.f, 1.0f, .5f, 1.f),
        0.f,
        all_params[NosParams::NoisePortaType],
        [this] (float val)
        {for (auto mod: modules) tNoiseModule_setParameter(mod, NoisePortaType,val);
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

};

class NoiseModuleProcessor : public ProcessorStateBase<PluginStateImpl_<NoiseParams>>
{
public:
    NoiseModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf,juce::UndoManager*);

    electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const noexcept override {
        return electrosynth::audio::makeGeneratorDescriptor();
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
    void releaseResources() override {}
    //void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
    //    bool acceptsMidi() const override
    //    {
    //       return true;
    std::unique_ptr<SynthSection> createEditor() override
    {
        return std::make_unique<electrosynth::ParametersView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
    // juce::AudioProcessorEditor* createEditor() override {return new electrosynth::ParametersViewEditor{*this,vstate.getProperty(IDs::type).toString() + vstate.getProperty(IDs::uuid).toString()};};
    chowdsp::ScopedCallbackList callbacks;

    uint8_t noVoicesSounding = 1;

};

#endif // ELECTORSYNTH_NOISEMODULEPROCESSOR_H
