//
// Created by Davis Polito on 11/19/24.
//

#include "EnvModuleProcessor.h"

#include "sound_engine.h"
#include "leaf-midi.h"
EnvModuleProcessor::EnvModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree& vt, LEAF* leaf)
    :ModulatorStateBase(engine,leaf,vt )
{
   procArray = &state_.params.processors[0];
   vt.setProperty(IDs::uuid, state_.params.processors[0].processorUniqueID, nullptr);
   name = vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString();
}
juce::AudioBuffer<float>* EnvModuleProcessor::processMasterEnvelope() {
        static juce::AudioBuffer<float> temp_voice_buffer{MAX_NUM_VOICES*2,1};
        for (uint8_t i = 0; i < engine->voiceHandler.numVoicesActive; i++) {
            if (!engine->voiceHandler.voiceIsSounding[i]) continue;
            procArray[i].tick(procArray[i].object,nullptr);
            temp_voice_buffer.setSample(i*2,0, procArray[i].outParameters[0]);
            temp_voice_buffer.setSample(i*2+1,0, procArray[i].outParameters[0]);
            if (state_.params.modules[i]->theEnv->whichStage == env_idle) {
                tSimplePoly_deactivateVoice(engine->voiceHandler.voices[0],i);
                engine->voiceHandler.voiceIsSounding[i] = false;
            }
        }
    return &temp_voice_buffer;
}
