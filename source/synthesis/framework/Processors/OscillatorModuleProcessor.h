//
// Created by Davis Polito on 8/8/24.
//

#ifndef ELECTROSYNTH_OSCILLATORMODULEPROCESSOR_H
#define ELECTROSYNTH_OSCILLATORMODULEPROCESSOR_H
#include "SimpleOscModule.h"
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






//template <typename ParameterState,  typename T>
//class PluginStateImpl_ : public chowdsp::PluginStateImpl<ParameterState>
//{
//    static_assert (std::is_base_of_v<LEAFParams<T>, ParameterState>, "ParameterState must be a chowdsp::ParamHolder!");
//
//    PluginStateImpl_(T t, juce::AudioProcessor * proc,  juce::UndoManager* um = nullptr) : chowdsp::PluginStateImpl<ParameterState> (proc, um)
//    {
//        this->params(t);
//    }
//};
//typedef enum {
//    OscMidiPitch,
//    OscHarmonic,
//    OscPitchOffset,
//    OscPitchFine,
//    OscFreqOffset,
//    OscShapeParam,
//    OscAmpParam,
//    OscGlide,
//    OscStepped,
//    OscSyncMode,
//    OscSyncIn,
//    OscType,
//    OscNumParams
//} OscParams;

constexpr std::array<const char*, 13> OscParamNames = {
    "",       // OscMidiPitch
    "harmonic",           // OscHarmonic
    "pitch",              // OscPitchOffset
    "pitchfine",         // OscPitchFine
    "freqoffset",        // OscFreqOffset
    "shape",              // OscShapeParam
    "amp",                // OscAmpParam
    "glide",              // OscGlide
    "harmonicStepped",    // OscStepped
    "",                   // OscSyncMode (undefined in your parameters)
    "",                   // OscSyncIn (undefined in your parameters)
    "oscType",            // OscType
    ""                    // OscNumParams (typically represents the count, no corresponding parameter)
};
enum class FlagOscTypes{
    OscTypeSawSquare =1,
    OscTypeSineTri =2,
    OscTypeSaw =4,
    OscTypePulse = 8,
    OscTypeSine = 16,
    OscTypeTri = 32

} ;
struct OscillatorParams : public LEAFParams<_tOscModule >
{
    OscillatorParams(LEAF* leaf) : LEAFParams<_tOscModule>(leaf)
    {
       add(harmonic, pitchOffset, pitchFine, freqOffset, glide, shape, harmonicstepped, amp, oscType);
        //add(pitchOffset);

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


    chowdsp::FloatParameter::Ptr harmonic {
        juce::ParameterID{"harmonic" , 100},
        "Harmonic",
        chowdsp::ParamUtils::createNormalisableRange(-16.f, 16.f, 0.f),
        0.f,
        all_params[OscParams::OscHarmonic],
        [this](float val)
        {
            for (auto mod : modules)
                mod->setterFunctions[OscParams::OscHarmonic](mod,val);
        DBG("harm [0 - 1]" + juce::String(val) + " .. .  harm actual Val" + juce::String(modules[0]->harmonicMultiplier));
        },
        &electrosynth::utils::harmonicValToString,
        &electrosynth::utils::stringToHarmonicVal
    };
    chowdsp::FloatParameter::Ptr pitchOffset {
        juce::ParameterID{"pitch" , 100},
        "Pitch",
        chowdsp::ParamUtils::createNormalisableRange(-12.f, 12.f, 0.f,1.f),
        0.0f,
        all_params[OscParams::OscPitchOffset],
        [this](float val)
        { for (auto mod : modules)
            mod->setterFunctions[OscParams::OscPitchOffset](mod,val);
        DBG("pitch [0 - 1] " + juce::String(val)  + " ... pitch actual " + juce::String(modules[0]->pitchOffset));
       },

        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr pitchFine {
        juce::ParameterID{"pitchfine" , 100},
        "Pitch Fine",
        chowdsp::ParamUtils::createNormalisableRange(-1.f, 1.f,0.f),
        0.f,
        all_params[OscParams::OscPitchFine],
        [this]( float val)
        {for (auto mod : modules)
            mod->setterFunctions[OscParams::OscPitchFine](mod,val);
        DBG("fine [0 - 1] " + juce::String(val) + " ..... fine actual " + juce::String(modules[0]->fine));
    },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr freqOffset
        {
        juce::ParameterID{"freqoffset" , 100},
        "Freq Offset",
        chowdsp::ParamUtils::createNormalisableRange(-2000.f, 2000.f,0.f),
        0.0f,
        all_params[OscParams::OscFreqOffset],
        [this]( float val)
        {for (auto mod : modules)
            mod->setterFunctions[OscParams::OscFreqOffset](mod,val);
            DBG("freq [0 - 1] " + juce::String(val) + " .... freq actual Val" + juce::String(modules[0]->freqOffset));},
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };

    chowdsp::FloatParameter::Ptr glide
    {
        juce::ParameterID{"glide" , 100},
        "Freq Glide",
        chowdsp::ParamUtils::createNormalisableRange(0.f, 1.f,0.5f),
        0.0f,
        all_params[OscParams::OscGlide],
        [this]( float val)
        {for (auto mod : modules)
            mod->setterFunctions[OscParams::OscGlide](mod,val);
            //DBG("freq [0 - 1] " + juce::String(val) + " .... glide actual Val" + juce::String(mod->glide));
        },
        &chowdsp::ParamUtils::floatValToString,
        &chowdsp::ParamUtils::stringToFloatVal
    };
    chowdsp::FloatParameter::Ptr shape
        {
            juce::ParameterID{"shape" , 100},
            "Shape",
            chowdsp::ParamUtils::createNormalisableRange(0.0f, 1.f ,0.5f),
            0.5f,
            all_params[OscParams::OscShapeParam],
            [this]( float val)
            {
                for (auto mod : modules)
                    mod->setterFunctions[OscParams::OscShapeParam](mod->theOsc,val);
            DBG("sghape [0 - 1]" + juce::String(val) + ".... cant see actual val");
            },
            &chowdsp::ParamUtils::floatValToString,
            &chowdsp::ParamUtils::stringToFloatVal
        };
    chowdsp::FloatParameter::Ptr amp
        {
            juce::ParameterID{"amp" , 100},
            "amplitude",
            chowdsp::ParamUtils::createNormalisableRange(0.0f, 2.f ,1.f),
            0.8f,
            all_params[OscParams::OscAmpParam],
            [this]( float val)
            {for (auto mod : modules)
                mod->setterFunctions[OscParams::OscAmpParam](mod,val);
            DBG("amp [0 - 1] " + juce::String(val) + ".. .... amp actual " + juce::String(modules[0]->amp));
            },
            &chowdsp::ParamUtils::floatValToString,
            &chowdsp::ParamUtils::stringToFloatVal
        };
    chowdsp::BoolParameter::Ptr harmonicstepped
    {
        juce::ParameterID{"harmonicStepped" , 100},
        "Harmonic stepped",
        0.0,
        all_params[OscParams::OscSteppedHarmonic],
        [this]( float val)
        {for (auto mod : modules)
            mod->setterFunctions[OscParams::OscSteppedHarmonic](mod,val);
            //harmonic->range.interval = val;
            DBG("amp [0 - 1] " + juce::String(val) + ".. .... stepped harmonic actual " + juce::String(modules[0]->hStepped));
        }
    };

    chowdsp::EnumChoiceParameter<FlagOscTypes>::Ptr oscType {
        juce::ParameterID{"oscType" , 100},
        "Oscillator Type",
        FlagOscTypes::OscTypePulse,
        all_params[OscParams::OscType],
        [this](float val){
        //{module->setterFunctions[OscParams::OscType](mod,val);
           // DBG("harm [0 - 1]" + juce::String(val) + " .. .  harm actual Val" + juce::String(mod->harmonicMultiplier));
        }
    };
    chowdsp::BoolParameter::Ptr pitchStepped
{
    juce::ParameterID{"pitchStepped" , 100},
    "Harmonic stepped",
    0.0,
    all_params[OscParams::OscSteppedPitch],
    [this]( float val)
    {for (auto mod : modules)
        mod->setterFunctions[OscParams::OscSteppedPitch](mod,val);
        //harmonic->range.interval = val;
        DBG("amp [0 - 1] " + juce::String(val) + ".. .... stepped pitch actual " + juce::String(modules[0]->pStepped));
    }
};




};
class OscillatorModuleProcessor : public ProcessorStateBase<PluginStateImpl_<OscillatorParams>>
{
public:
    OscillatorModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree&, LEAF* leaf);

    void getNextAudioBlock (const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void prepareToPlay (int samplesPerBlock, double sampleRate ) override {};
    void releaseResources() override {}
    //void processAudioBlock (juce::AudioBuffer<float>& buffer) override {};
//    bool acceptsMidi() const override
//    {
//       return true;
 electrosynth::ParametersView* createEditor() override
    {
        return new electrosynth::ParametersView(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());
    }
   // juce::AudioProcessorEditor* createEditor() override {return new electrosynth::ParametersViewEditor{*this,vstate.getProperty(IDs::type).toString() + vstate.getProperty(IDs::uuid).toString()};};
    chowdsp::ScopedCallbackList callbacks;

};

#endif //ELECTROSYNTH_OSCILLATORMODULEPROCESSOR_H
