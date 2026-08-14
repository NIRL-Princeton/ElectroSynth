//
// Created by Matthew McWeeney on 8/14/26.
//

#include "VCAModuleProcessor.h"
#include "sound_engine.h"

VCAModuleProcessor::VCAModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) : ProcessorStateBase(engine,leaf,v,um)
{
}

void VCAModuleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    int numSamples = buffer.getNumSamples();

    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        auto* L = buffer.getWritePointer(v*2);
        auto* R = buffer.getWritePointer(v*2 +1);
        for (int i = 0; i < numSamples; i++)
        {
            tVCAModule_tick(state_.params.modules[v],L);
            R[i] = L[i];
        }

    }
}
