//
// Created by Davis Polito on 8/8/24.
//

#include "FilterModuleProcessor.h"
#include "Identifiers.h"
#include "Identifiers.h"
//float electrosynth::utils::stringToHarmonicVal(const juce::String &s){
//    if(!s.contains("/"))
//    {
//        return s.getFloatValue();
//    }
//    else
//    {
//        juce::StringArray tokens;
//        tokens.addTokens(s,"/","\"");
//        return tokens[1].getFloatValue();
//    }
//}
//
//juce::String electrosynth::utils::harmonicValToString(float harmonic)
//{
//    if(harmonic < 0.f)
//        return "1 / " + juce::String(abs(harmonic));
//    else
//        return juce::String(harmonic);
//}
FilterModuleProcessor::FilterModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) : ProcessorStateBase(engine,leaf,v,um)
{
}
#include "sound_engine.h"
void FilterModuleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    int numSamples = buffer.getNumSamples();
    const int requestedFilterType = juce::jlimit(0, (int)FiltNumTypes - 1,
                                                 state_.params.filterType->getIndex());
    if (requestedFilterType != currentFilterType_) {
        for (auto* module : state_.params.modules)
            tFiltModule_setType(module, (uint32_t)requestedFilterType,
                                (uint32_t)filterTransitionSamples_);
        currentFilterType_ = requestedFilterType;
    }
    //buffer.clear();
    //    auto* samplesL = buffer.getReadPointer(0);

    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        tFiltModule_setParameter(state_.params.modules[v], FiltMidiPitch,engine->voiceHandler.voiceNote[v]/127.f );
        auto* L = buffer.getWritePointer(v*2);
        auto* R = buffer.getWritePointer(v*2 +1);
        for (int i = 0; i < numSamples; i++)
        {
           tFiltModule_tick (state_.params.modules[v],L);

            R[i] = L[i];
        }

    }
}
