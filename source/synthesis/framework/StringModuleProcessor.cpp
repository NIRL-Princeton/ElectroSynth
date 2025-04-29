//
// Created by Davis Polito on 8/8/24.
//

#include "StringModuleProcessor.h"
#include "Identifiers.h"

void StringModuleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    state.getParameterListeners().callAudioThreadBroadcasters();
    int numSamples = buffer.getNumSamples();
    //buffer.clear();

//    auto* samplesL = buffer.getReadPointer(0);
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    for (int i = 0; i < numSamples; i++)
    {
        tStringModule_tick(state.params.module,L);
        L[i] += state.params.module->outputs[0];
        R[i] = L[i];
    }

}