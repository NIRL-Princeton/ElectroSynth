//
// Created by Davis Polito on 8/8/24.
//

#ifndef ELECTROSYNTH_STRINGMODULEPROCESSOR_H
#define ELECTROSYNTH_STRINGMODULEPROCESSOR_H
#include "StringModule.h"
#include "../PluginStateImpl_.h"
#include "ParameterView/ParametersView.h"
#include "Identifiers.h"
#include "ProcessorBase.h"
namespace electrosynth{
    namespace utils
    {
        float stringToHarmonicVal(const juce::String &s);
        juce::String harmonicValToString(float harmonic);
    }
}

struct StringParams : public LEAFParams<_tStringModule>
{
    StringParams(LEAF* leaf) : LEAFParams<_tStringModule>(leaf)
    {
        add(
            oversample, freq, waveLength, dampFreq, decay, targetLevel,
            levelSmooth, levelStrength, pickupPoint, levelMode, rippleGain,
            rippleDelay, pluckPosition
        );
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
    chowdsp::FloatParameter::Ptr oversample {
        juce::ParameterID{"oversample", 100},
        "Oversample",
        chowdsp::ParamUtils::createNormalisableRange(2.f,4.f, 3.f, 2.0f),
        2.f,
        all_params[StringOversample],
        [this](float val) { for (auto mod: modules)
            mod->setterFunctions[StringOversample](mod, val); }
        ,    &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr freq {
        juce::ParameterID{"freq", 100},
        "Frequency",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
      0.5f,
        all_params[StringFreq],
        [this](float val) {
            for (auto mod: modules) mod->setterFunctions[StringFreq](mod, val);
        },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr waveLength {
        juce::ParameterID{"waveLength", 100},
        "Wave Length",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
        0.5f,
        all_params[StringWaveLength],
        [this](float val) { for (auto mod: modules)  mod->setterFunctions[StringWaveLength](mod, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr dampFreq {
        juce::ParameterID{"dampFreq", 100},
        "Damping Frequency",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
      0.5f,
        all_params[StringDampFreq],
        [this](float val) { for (auto mod: modules) mod->setterFunctions[StringDampFreq](mod, val); },    &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr decay {
        juce::ParameterID{"decay", 100},
        "Decay",
        chowdsp::ParamUtils::createNormalisableRange(0.01f, 1.f, 0.5f),
        0.5f,
        all_params[StringDecay],
        [this](float val) { for (auto mod: modules)  mod->setterFunctions[StringDecay](mod, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr targetLevel {
        juce::ParameterID{"targetLevel", 100},
        "Target Level",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
        0.5f,
        all_params[StringTargetLevel],
        [this](float val) { for (auto mod: modules)  mod->setterFunctions[StringTargetLevel](mod, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr levelSmooth {
        juce::ParameterID{"levelSmooth", 100},
        "Level Smooth",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
      0.5f,
        all_params[StringLevelSmooth],
        [this](float val) { for (auto mod: modules)  mod->setterFunctions[StringLevelSmooth](mod, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr levelStrength {
        juce::ParameterID{"levelStrength", 100},
        "Level Strength",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
      0.5f,
        all_params[StringLevelStrength],
        [this](float val) { for (auto mod: modules)  mod->setterFunctions[StringLevelStrength](mod, val); }
        ,    &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr pickupPoint {
        juce::ParameterID{"pickupPoint", 100},
        "Pickup Point",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
      0.5f,
        all_params[StringPickupPoint],
        [this](float val) { for (auto mod: modules) mod->setterFunctions[StringPickupPoint](mod, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::BoolParameter::Ptr levelMode {
        juce::ParameterID{"levelMode", 100},
        "Level Mode",
        false,
        all_params[StringLevelMode],
        [this](float val) { for (auto mod: modules)  mod->setterFunctions[StringLevelMode](mod, val); }
    };

    chowdsp::FloatParameter::Ptr rippleGain {
        juce::ParameterID{"rippleGain", 100},
        "Ripple Gain",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
     0.5f,
        all_params[StringRippleGain],
        [this](float val) { for (auto mod: modules)  mod->setterFunctions[StringRippleGain](mod, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr rippleDelay {
        juce::ParameterID{"rippleDelay", 100},
        "Ripple Delay",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
       0.5f,
        all_params[StringRippleDelay],
        [this](float val) { for (auto mod: modules)  mod->setterFunctions[StringRippleDelay](mod, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr pluckPosition {
        juce::ParameterID{"pluckPosition", 100},
        "Pluck Position",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
      0.5f,
        all_params[StringPluckPosition],
        [this](float val) { for (auto mod: modules)  mod->setterFunctions[StringPluckPosition](mod, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };
};


class StringModuleProcessor : public ProcessorStateBase<PluginStateImpl_<StringParams>>
{
public:
    StringModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree& _state, LEAF* leaf)
        : ProcessorStateBase(engine,leaf,_state)
    {
        state.setProperty(IDs::uuid, state_.params.processors[0].processorUniqueID, nullptr);
        name = state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString();
        procArray = &state_.params.processors[0];
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    void prepareToPlay(int samplesPerBlock, double sampleRate) override
    {
        // Implement if necessary
    }

    void releaseResources() override {}

    electrosynth::ParametersView* createEditor() override
    {
        return new electrosynth::ParametersView(
            state_,
            state_.params,
            state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString()
        );
    }

    chowdsp::ScopedCallbackList callbacks;
};



#endif //ELECTROSYNTH_OSCILLATORMODULEPROCESSOR_H
