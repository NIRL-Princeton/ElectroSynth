//
// Created by Myra Norton on 1/22/26.
//

#include "DelayModuleProcessor.h"
#include "sound_engine.h"

DelayModuleProcessor::DelayModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) : ProcessorStateBase(engine,leaf,v,um)
{
}

void DelayModuleProcessor::process ()
{
    int numSamples = 1;
    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        for (int i = 0; i < numSamples; i++)
        {
            float dummy = 0.f;
            tDelayModule_tick(state_.params.modules[v], &dummy);
        }
    }
}
