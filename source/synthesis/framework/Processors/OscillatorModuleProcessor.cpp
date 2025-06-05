//
// Created by Davis Polito on 8/8/24.
//

#include "OscillatorModuleProcessor.h"
#include "Identifiers.h"
#include "sound_engine.h"
float electrosynth::utils::stringToHarmonicVal(const juce::String &s){
    if(!s.contains("/"))
    {
        return s.getFloatValue();
    }
    else
    {
        juce::StringArray tokens;
        tokens.addTokens(s,"/","\"");
        return tokens[1].getFloatValue();
    }
}

juce::String electrosynth::utils::harmonicValToString(float harmonic)
{
    if(harmonic < 0.f)
        return "1 / " + juce::String(abs(round(harmonic)));
    else
        return juce::String(round(harmonic));
}
OscillatorModuleProcessor::OscillatorModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf) :ProcessorStateBase(engine,leaf,v)


{

    callbacks += {
        state_.addParameterListener (*state_.params.oscType, chowdsp::ParameterListenerThread::AudioThread, [this] {
            auto theType = state_.params.oscType.get();
            float val =  (float)theType->getIndex() / (float)OscTypes::OscNumTypes;
            for (auto mod: state_.params.modules) {
                mod->setterFunctions[OscParams::OscType](mod,val);
                mod->setterFunctions[OscParams::OscShapeParam](mod->theOsc, *mod->params[OscShapeParam]);
            //also need to update the shape since the new oscillator type will default to its initial shape instead
            }



        })
    };
    state.setProperty(IDs::uuid, state_.params.processors[0].processorUniqueID, nullptr);
    name = state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString();
    procArray = &state_.params.processors[0];

   //tOscModule_init(static_cast<void*>(module), {0, 0}, id, leaf)
    //tOscModule_processorInit(state_.params.module, &processor);
}

void OscillatorModuleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    state_.getParameterListeners().callAudioThreadBroadcasters();
    int numSamples = buffer.getNumSamples();
    //buffer.clear();

    //    auto* samplesL = buffer.getReadPointer(0);
    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        if (!engine->voiceHandler.voiceIsSounding[v]) continue;
        procArray[v].setterFunctions[OscParams::OscMidiPitch](procArray[v].object,engine->voiceHandler.voiceNote[v]/127.f);
        auto* L = buffer.getWritePointer(v*2);
        auto* R = buffer.getWritePointer(v*2+1);
        for (int i = 0; i < numSamples; i++)
        {
            procArray[v].tick(procArray[v].object,L);
            L[i] += procArray[v].outParameters[0];
            R[i] = L[i];
        }
    }
    // ProcessorBase::processBlock(buffer,midi);
}