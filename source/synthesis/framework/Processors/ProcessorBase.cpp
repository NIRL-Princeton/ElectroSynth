//
// Created by Davis Polito on 11/19/24.
//

#include "ProcessorBase.h"

#include "sound_engine.h"

void ProcessorBase::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &) {

    int numSamples = buffer.getNumSamples();
    //buffer.clear();

    //    auto* samplesL = buffer.getReadPointer(0);
    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        auto* L = buffer.getWritePointer(0);
        auto* R = buffer.getWritePointer(1);
        for (int i = 0; i < numSamples; i++)
        {
            procArray[v].tick(procArray[v].object,L);
            L[i] += procArray[v].outParameters[0];
            R[i] = L[i];
        }
    }

}
