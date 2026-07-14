//
// Created by Matthew McWeeney on 7/14/26.
//

#include "SimpleNoiseModuleProcessor.h"
#include "sound_engine.h"
#include  "LFOModule.h"
SimpNoiseModuleProcessor::SimpNoiseModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree& vt, LEAF* leaf,juce::UndoManager *um)
    :ModulatorStateBase(engine,leaf,vt ,um)
{
}

void SimpNoiseModuleProcessor::process() {
    for (int i = 0; i < engine->voiceHandler.numVoicesActive; i++) {
        tSimpNoiseModule_tick(state_.params.modules[i]);
    }
}