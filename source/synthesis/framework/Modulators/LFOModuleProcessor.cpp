//
// Created by Davis Polito on 1/27/25.
//

#include "LFOModuleProcessor.h"
#include "sound_engine.h"
#include  "LFOModule.h"
LFOModuleProcessor::LFOModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree& vt, LEAF* leaf,juce::UndoManager *um)
    :ModulatorStateBase(engine,leaf,vt ,um)
{

}

void LFOModuleProcessor::process() {
    for (int i = 0; i < engine->voiceHandler.numVoicesActive; i++) {
        tLFOModule_tick(state_.params.modules[i]);
    }
}
