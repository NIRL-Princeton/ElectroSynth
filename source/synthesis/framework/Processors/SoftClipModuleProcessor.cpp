//
// Created by Jeff Snyder on 1/7/26.
//

#include "SoftClipModuleProcessor.h"


SoftClipModuleProcessor::SoftClipModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) : ProcessorStateBase(engine,leaf,v,um)
{
   //tOscModule_init(static_cast<void*>(module), {0, 0}, id, leaf)
    //tFiltModule_processorInit(state_.params.module, &processor);
   state.setProperty(IDs::uuid, state_.params.processors[0].processorUniqueID, nullptr);
    procArray = &state_.params.processors[0];
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
            procArray[v].tick(procArray[v].object,L);

            R[i] = L[i];
        }

    }
}