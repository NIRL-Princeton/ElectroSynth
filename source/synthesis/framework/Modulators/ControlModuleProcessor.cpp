//
// Created by Jeff Snyder on 11/15/26.
//
#include "ControlModuleProcessor.h"

#include "sound_engine.h"
//#include "leaf-midi.h"
#include "ControlModule.h"
ControlModuleProcessor::ControlModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree& vt, LEAF* leaf, juce::UndoManager *um)
    :ModulatorStateBase(engine,leaf,vt , um)
{
}
void ControlModuleProcessor::process() {
    for (int i = 0; i < engine->voiceHandler.numVoicesActive; i++) {
        if (!engine->voiceHandler.voiceIsSounding[i]) continue;
        //get CC value
        float myCCValue = engine->ccArray[state_.params.modules[i]->ccNum+ 30] * 0.00787402; //divide by 127
        tControlModule_setParameter(state_.params.modules[i], ControlCCValue,myCCValue);
        tControlModule_tick(state_.params.modules[i]);
    }
}

