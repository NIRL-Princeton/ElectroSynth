//
// Created by Myra Norton on 7/25/25.
//

#include "DelayModuleProcessor.h"
#include "Identifiers.h"
#include "sound_engine.h"

DelayModuleProcessor::DelayModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) : ProcessorStateBase(engine,leaf,v,um)
{
    state.setProperty(IDs::uuid, state_.params.processors[0].processorUniqueID, nullptr);
    procArray = &state_.params.processors[0];
}

void DelayModuleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    int numSamples = buffer.getNumSamples();

    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        if (!engine->voiceHandler.voiceIsSounding[v]) continue;

        auto* L = buffer.getWritePointer(v*2);
        auto* R = buffer.getWritePointer(v*2 +1);

        for (int i = 0; i < numSamples; i++)
        {
            procArray[v].tick(procArray[v].object,L);
            R[i] = L[i];
        }
    }
}
