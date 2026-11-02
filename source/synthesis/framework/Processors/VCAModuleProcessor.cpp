//
// Created by Matthew McWeeney on 8/14/26.
//

#include "VCAModuleProcessor.h"
#include "sound_engine.h"

VCAModuleProcessor::VCAModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) : ProcessorStateBase(engine,leaf,v,um)
{
}

void VCAModuleProcessor::process()
{
    int numSamples = 1;

    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        for (int i = 0; i < numSamples; i++)
        {
            float dummy = 0.f;
            tVCAModule_tick(state_.params.modules[v], &dummy);
        }

    }
}
