//
// Created by Jeff Snyder on 1/7/26.
//

#include "SoftClipModuleProcessor.h"


SoftClipModuleProcessor::SoftClipModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) : ProcessorStateBase(engine,leaf,v,um)
{
}
#include "sound_engine.h"
void SoftClipModuleProcessor::process()
{
    int numSamples = 1;
    //buffer.clear();
    //    auto* samplesL = buffer.getReadPointer(0);

    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        for (int i = 0; i < numSamples; i++)
        {
            float dummy = 0.f;
            tSoftClipModule_tick(state_.params.modules[v], &dummy);
        }

    }
}
