//
// Created by Matthew McWeeney on 8/28/26.
//

#include "SampleAndHoldProcessor.h"
#include "sound_engine.h"



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