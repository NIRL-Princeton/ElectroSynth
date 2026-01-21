//
// Created by Jeff Snyder on 1/7/26.
//

#include "SoftClipModuleProcessor.h"


SoftClipModuleProcessor::SoftClipModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) : ProcessorStateBase(engine,leaf,v,um)
{
}
#include "sound_engine.h"
void SoftClipModuleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    int numSamples = buffer.getNumSamples();
    //buffer.clear();
    //    auto* samplesL = buffer.getReadPointer(0);

    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        auto* L = buffer.getWritePointer(v*2);
        auto* R = buffer.getWritePointer(v*2 +1);
        for (int i = 0; i < numSamples; i++)
        {
            tSoftClipModule_tick(state_.params.modules[v],L);

            R[i] = L[i];
        }

    }
}
