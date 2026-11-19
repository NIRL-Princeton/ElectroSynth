//
// Created by Matthew McWeeney on 8/28/26.
//

#include "SampleAndHoldProcessor.h"
#include "sound_engine.h"

float electrosynth::utils::stringToHarmonicVal2(const juce::String &s){
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

juce::String electrosynth::utils::harmonicValToString2(float harmonic)
{
    if(harmonic < 0.f)
        return "1 / " + juce::String(abs(round(harmonic) - 1.f)) ;
    else
        return juce::String(round(harmonic + 1.f));
}

SampleAndHoldProcessor::SampleAndHoldProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) : ProcessorStateBase(engine,leaf,v,um)
{
}

void SampleAndHoldProcessor::process() {
    int numSamples = 1;
    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        tSampleAndHoldModule_setParameter(state_.params.modules[v], SampHoldMidiPitch,engine->voiceHandler.voiceNote[v]/127.f );
        for (int i = 0; i < numSamples; i++) {
            float input = 0.f;
            tSampleAndHoldModule_tick (state_.params.modules[v]);
        }

    }
}