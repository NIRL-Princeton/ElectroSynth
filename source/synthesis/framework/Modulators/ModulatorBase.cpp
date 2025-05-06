//
// Created by Davis Polito on 11/19/24.
//

#include "ModulatorBase.h"
#include "sound_engine.h"
void ModulatorBase::process() {
    for (int i = 0; i < engine->voiceHandler.numVoicesActive; i++) {
        procArray[i].tick(procArray[i].object,nullptr);
    }
}
