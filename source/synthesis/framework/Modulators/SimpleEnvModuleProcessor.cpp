//
// Created by Matthew McWeeney on 9/3/26.
//

#include "SimpleEnvModuleProcessor.h"
#include "EnvModuleProcessor.h"

#include "sound_engine.h"
//#include "leaf-midi.h"

SimpleEnvModuleProcessor::SimpleEnvModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree& vt, LEAF* leaf, juce::UndoManager *um)
    :ModulatorStateBase(engine,leaf,vt , um)
{
}
// juce::AudioBuffer<float>* SimpleEnvModuleProcessor::processMasterEnvelope() {
//     static juce::AudioBuffer<float> temp_voice_buffer{MAX_NUM_VOICES*2,1};
//     for (uint8_t i = 0; i < engine->voiceHandler.numVoicesActive; i++) {
//         if (!engine->voiceHandler.voiceIsSounding[i]) continue;
//         tSimpleEnvModule_tick(state_.params.modules[i]);
//         temp_voice_buffer.setSample(i*2,0, state_.params.modules[i]->header.outputs[0]);
//         temp_voice_buffer.setSample(i*2+1,0, state_.params.modules[i]->header.outputs[0]);
//         if (state_.params.modules[i]->theEnv.whichStage == env_idle) {
//             tSimplePoly_deactivateVoice(engine->voiceHandler.voices,i);
//             //tSimplePoly_deactivateVoice(engine->voiceHandler.voices[0],i);
//             engine->voiceHandler.voiceIsSounding[i] = false;
//         }
//     }
//     return &temp_voice_buffer;
// }


void SimpleEnvModuleProcessor::process() {
    for (int i = 0; i < engine->voiceHandler.numVoicesActive; i++) {
        if (!engine->voiceHandler.voiceIsSounding[i]) continue;
        tSimpleEnvModule_tick(state_.params.modules[i]);
    }
}
