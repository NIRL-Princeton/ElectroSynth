//
// Created by Davis Polito on 8/8/24.
//

#include "StringModuleProcessor.h"
#include "Identifiers.h"
#include "sound_engine.h"
void StringModuleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    state_.getParameterListeners().callAudioThreadBroadcasters();
    int numSamples = buffer.getNumSamples();
    //buffer.clear();

//    auto* samplesL = buffer.getReadPointer(0);
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        for (int i = 0; i < numSamples; i++)
        {

            tStringModule_tick(state_.params.modules[v],L);
            L[i] += state_.params.modules[v]->header.outputs[v];
            R[i] = L[i];
        }
    }
}
