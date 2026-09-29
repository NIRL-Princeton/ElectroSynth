//
// Created by MrKahoobadoo on 9/24/26.
//

#include "RandAndHoldProcessor.h"
#include "sound_engine.h"

// RandAndHoldProcessor::RandAndHoldProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) : ModulatorStateBase(engine,leaf,v,um)
// {
// }
//
// void RandAndHoldProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
//     int numSamples = buffer.getNumSamples();
//
//     for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
//         auto* L = buffer.getWritePointer(v*2);
//         auto* R = buffer.getWritePointer(v*2 +1);
//         for (int i = 0; i < numSamples; i++)
//         {
//             tRandAndHoldModule_tick (state_.params.modules[v],L);
//             R[i] = L[i];
//         }
//     }
// }

RandAndHoldProcessor::RandAndHoldProcessor(electrosynth::SoundEngine* engine,juce::ValueTree& vt, LEAF* leaf,juce::UndoManager *um)
    :ModulatorStateBase(engine,leaf,vt ,um)
{
}
//
// void RandAndHoldProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
//     int numSamples = buffer.getNumSamples();
//
//     for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
//         auto* L = buffer.getWritePointer(v*2);
//         auto* R = buffer.getWritePointer(v*2 +1);
//         for (int i = 0; i < numSamples; i++)
//         {
//             tRandAndHoldModule_tick (state_.params.modules[v],L);
//             R[i] = L[i];
//         }
//     }
// }

void RandAndHoldProcessor::process() {
    for (int i = 0; i < engine->voiceHandler.numVoicesActive; i++) {
        tRandAndHoldModule_tick(state_.params.modules[i]);
    }
}
