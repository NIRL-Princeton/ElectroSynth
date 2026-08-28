//
// Created by Matthew McWeeney on 8/28/26.
//

#include "SampleAndHoldProcessor.h"
#include "sound_engine.h"

SampleAndHoldProcessor::SampleAndHoldProcessor(electrosynth::SoundEngine* engine,juce::ValueTree& vt, LEAF* leaf,juce::UndoManager *um)
    :ModulatorStateBase(engine,leaf,vt ,um)
{
}

void SampleAndHoldProcessor::process() {
    for (int i = 0; i < engine->voiceHandler.numVoicesActive; i++) {
        tSampleAndHoldModule_tick(state_.params.modules[i]);
    }
}