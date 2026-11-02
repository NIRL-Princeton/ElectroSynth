//
// Created by Davis Polito on 8/8/24.
//

#include "StringModuleProcessor.h"
#include "Identifiers.h"
#include "sound_engine.h"
void StringModuleProcessor::process ()
{
    state_.getParameterListeners().callAudioThreadBroadcasters();
    int numSamples = 1;
    //buffer.clear();

//    auto* samplesL = buffer.getReadPointer(0);

    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        tStringModule_setParameter(state_.params.modules[v], StringMidiFreq,mtof (engine->voiceHandler.voiceNote[v]/127.f));
        for (int i = 0; i < numSamples; i++)
        {
            float dummy = 0.f;
            tStringModule_tick(state_.params.modules[v], &dummy);
        }
    }
}
