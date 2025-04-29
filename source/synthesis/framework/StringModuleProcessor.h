//
// Created by Davis Polito on 8/8/24.
//

#ifndef ELECTROSYNTH_STRINGMODULEPROCESSOR_H
#define ELECTROSYNTH_STRINGMODULEPROCESSOR_H
#include "StringModule.h"
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



    chowdsp::FloatParameter::Ptr oversample {
        juce::ParameterID{"oversample", 100},
        "Oversample",
        chowdsp::ParamUtils::createNormalisableRange(2.f,4.f, 3.f, 2.0f),
        2.f,
        &module->params[StringOversample],
        [this](float val) { module->setterFunctions[StringOversample](this->module, val); }
        ,    &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr freq {
        juce::ParameterID{"freq", 100},
        "Frequency",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
      0.5f,
        &module->params[StringFreq],
        [this](float val) {
            module->setterFunctions[StringFreq](this->module, val);
        },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr waveLength {
        juce::ParameterID{"waveLength", 100},
        "Wave Length",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
        0.5f,
        &module->params[StringWaveLength],
        [this](float val) { module->setterFunctions[StringWaveLength](this->module, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr dampFreq {
        juce::ParameterID{"dampFreq", 100},
        "Damping Frequency",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
      0.5f,
        &module->params[StringDampFreq],
        [this](float val) { module->setterFunctions[StringDampFreq](this->module, val); },    &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr decay {
        juce::ParameterID{"decay", 100},
        "Decay",
        chowdsp::ParamUtils::createNormalisableRange(0.01f, 1.f, 0.5f),
        0.5f,
        &module->params[StringDecay],
        [this](float val) { module->setterFunctions[StringDecay](this->module, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr targetLevel {
        juce::ParameterID{"targetLevel", 100},
        "Target Level",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
        0.5f,
        &module->params[StringTargetLevel],
        [this](float val) { module->setterFunctions[StringTargetLevel](this->module, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr levelSmooth {
        juce::ParameterID{"levelSmooth", 100},
        "Level Smooth",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
      0.5f,
        &module->params[StringLevelSmooth],
        [this](float val) { module->setterFunctions[StringLevelSmooth](this->module, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr levelStrength {
        juce::ParameterID{"levelStrength", 100},
        "Level Strength",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
      0.5f,
        &module->params[StringLevelStrength],
        [this](float val) { module->setterFunctions[StringLevelStrength](this->module, val); }
        ,    &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr pickupPoint {
        juce::ParameterID{"pickupPoint", 100},
        "Pickup Point",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
      0.5f,
        &module->params[StringPickupPoint],
        [this](float val) { module->setterFunctions[StringPickupPoint](this->module, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::BoolParameter::Ptr levelMode {
        juce::ParameterID{"levelMode", 100},
        "Level Mode",
        false,
        &module->params[StringLevelMode],
        [this](float val) { module->setterFunctions[StringLevelMode](this->module, val); }
    };

    chowdsp::FloatParameter::Ptr rippleGain {
        juce::ParameterID{"rippleGain", 100},
        "Ripple Gain",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
     0.5f,
        &module->params[StringRippleGain],
        [this](float val) { module->setterFunctions[StringRippleGain](this->module, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr rippleDelay {
        juce::ParameterID{"rippleDelay", 100},
        "Ripple Delay",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
       0.5f,
        &module->params[StringRippleDelay],
        [this](float val) { module->setterFunctions[StringRippleDelay](this->module, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr pluckPosition {
        juce::ParameterID{"pluckPosition", 100},
        "Pluck Position",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f, 0.5f),
      0.5f,
        &module->params[StringPluckPosition],
        [this](float val) { module->setterFunctions[StringPluckPosition](this->module, val); },
        &chowdsp::ParamUtils::floatValToString,
    &chowdsp::ParamUtils::stringToFloatVal
    };
};


class StringModuleProcessor : public ProcessorStateBase<PluginStateImpl_<StringParams, _tStringModule>>
{
public:
    StringModuleProcessor(const juce::ValueTree& _vt, LEAF* leaf)
        : ProcessorStateBase(leaf,_vt)
    {
        vt.setProperty(IDs::uuid, state.params.processor.processorUniqueID, nullptr);
        name = vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString();
        proc = &state.params.processor;
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
            state,
            state.params,
            vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString()
        );
    }

    chowdsp::ScopedCallbackList callbacks;
};



#endif //ELECTROSYNTH_OSCILLATORMODULEPROCESSOR_H
