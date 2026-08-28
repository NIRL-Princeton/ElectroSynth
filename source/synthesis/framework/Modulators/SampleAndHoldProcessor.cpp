//
// Created by Matthew McWeeney on 8/28/26.
//

#include "SampleAndHoldProcessor.h"
#include "sound_engine.h"

SampleAndHoldProcessor::SampleAndHoldProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) : ProcessorStateBase(engine,leaf,v,um)
{
}
#include "sound_engine.h"
void SampleAndHoldProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    int numSamples = buffer.getNumSamples();
    // const int requestedFilterType = juce::jlimit(0, (int)FiltNumTypes - 1,
    //                                              juce::roundToInt(state_.params.filterType->get()));
    // if (requestedFilterType != currentFilterType_) {
    //     for (auto* module : state_.params.modules)
    //         tFiltModule_setType(module, requestedFilterType);
    //     currentFilterType_ = requestedFilterType;
    // }
    //buffer.clear();
    //    auto* samplesL = buffer.getReadPointer(0);

    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        //tSampleAndHoldModule_setParameter(state_.params.modules[v], FiltMidiPitch,engine->voiceHandler.voiceNote[v]/127.f);
        auto* L = buffer.getWritePointer(v*2);
        auto* R = buffer.getWritePointer(v*2 +1);
        for (int i = 0; i < numSamples; i++)
        {
            tSampleAndHoldModule_tick (state_.params.modules[v],L);

            R[i] = L[i];
        }

    }
}