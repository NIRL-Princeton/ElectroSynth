//
// Created by MrKahoobadoo on 9/24/26.
//

#include "RandAndHoldProcessor.h"
#include "sound_engine.h"

float electrosynth::utils::stringToHarmonicVal3(const juce::String &s){
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

juce::String electrosynth::utils::harmonicValToString3(float harmonic)
{
    if(harmonic < 0.f)
        return "1 / " + juce::String(abs(round(harmonic) - 1.f)) ;
    else
        return juce::String(round(harmonic + 1.f));
}

RandAndHoldProcessor::RandAndHoldProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) : ProcessorStateBase(engine,leaf,v,um)
{
}

void RandAndHoldProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    int numSamples = buffer.getNumSamples();

    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        auto* L = buffer.getWritePointer(v*2);
        auto* R = buffer.getWritePointer(v*2 +1);
        for (int i = 0; i < numSamples; i++)
        {
            tRandAndHoldModule_tick (state_.params.modules[v],L);

            R[i] = L[i];
        }

    }
}