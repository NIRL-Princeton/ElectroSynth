//
// Created by Davis Polito on 7/11/25.
//

#ifndef VCAMODULEPROCESSOR_H
#define VCAMODULEPROCESSOR_H
#include "PluginStateImpl_.h"
#include "VCAModule.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "mapping.h"
#include "ProcessorBase.h"

enum RoutingMode {
    Master = 1 << 0,
    Lane_1 = 1 << 1,
    Lane_2 = 1 << 2,
    Lane_3 = 1 << 3,
};

struct RoutingParams : public LEAFParams<_tVCAModule> {
    RoutingParams(LEAF *leaf) : LEAFParams(leaf) {

        add(gainparam, routing);
        int i= 0;
        for (auto& ptr : all_params[VCAParams::VCAAudioInput]) {
            *ptr = &audio_in[i++];

        }
    }

    chowdsp::GainDBParameter::Ptr gainparam{
        juce::ParameterID{"gain", 100},
        "Gain",
        juce::NormalisableRange{-30.0f, 0.0f},
        0.f,
        all_params[VCAParams::VCAGain],
        [this](float val) {
            for (auto mod: modules)
                mod->header.setterFunctions[VCAParams::VCAGain](mod, val);
            //harmonic->range.interval = val;
            // DBG("amp [0 - 1] " + juce::String(val) + ".. .... stepped pitch actual " + juce::String(modules[0]->pStepped));
        }
    };

    chowdsp::EnumChoiceParameter<RoutingMode>::Ptr routing {
        juce::ParameterID{"reffundamental", 100},
                "RefFundamental",
                RoutingMode::Master,
                all_params[VCAParams::VCARouting],
                [this](float val) {
                    // for (auto mod: modules) {
                    //     //mod->setterFunctions[VCAParams::VCAGain](mod,val);
                    //     //harmonic->range.interval = val;
                    //     // DBG("amp [0 - 1] " + juce::String(val) + ".. .... stepped pitch actual " + juce::String(modules[0]->pStepped));
                    // }
                }
                    ,
                    std::initializer_list<std::pair<char, char> >{{'_', ' '}}
                };
    std::array<std::atomic<float>,MAX_NUM_VOICES> audio_in{0.f};
    };

    class RoutingProcessor : public ProcessorStateBase<PluginStateImpl_<RoutingParams> > {
    public:
        RoutingProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf, juce::UndoManager *um);
        void getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override {
        }

        void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &);

        void prepareToPlay(int samplesPerBlock, double sampleRate) override {
        };

        void releaseResources() override {
        }

        std::unique_ptr<SynthSection> createEditor() override;
        chowdsp::ScopedCallbackList callbacks;
        // std::array<leaf::tAudioRouting,MAX_NUM_VOICES> audio_routings;
        int curr_lane;
        juce::AudioBuffer<float> *audio_out;
    };


#endif //VCAMODULEPROCESSOR_H
