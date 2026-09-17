//
// Created by Matthew McWeeney on 9/6/26.
//

#include "ADEnvModuleProcessor.h"
#include "sound_engine.h"
//#include "leaf-midi.h"

ADEnvModuleProcessor::ADEnvModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree& vt, LEAF* leaf, juce::UndoManager *um)
    :ModulatorStateBase(engine,leaf,vt , um)
{
}

void ADEnvModuleProcessor::process() {
    for (int i = 0; i < engine->voiceHandler.numVoicesActive; i++) {
        if (!engine->voiceHandler.voiceIsSounding[i]) continue;
        tADEnvModule_tick(state_.params.modules[i]);
    }
}
