//
// Created by Matthew McWeeney on 7/15/26.
//

#include "PerlinNoiseModuleProcessor.h"
#include "sound_engine.h"
#include "PerlinNoiseModule.h"

PerlNoiseModuleProcessor::PerlNoiseModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree& vt, LEAF* leaf,juce::UndoManager *um)
    :ModulatorStateBase(engine,leaf,vt ,um)
{
}

void PerlNoiseModuleProcessor::process() {
    for (int i = 0; i < engine->voiceHandler.numVoicesActive; i++) {
        tPerlNoiseModule_tick(state_.params.modules[i]);
    }
}