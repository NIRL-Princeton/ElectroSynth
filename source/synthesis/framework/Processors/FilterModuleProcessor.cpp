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
void FilterModuleProcessor::process () {
    int numSamples = 1;
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
        tFiltModule_setParameter(state_.params.modules[v], FiltMidiPitch,engine->voiceHandler.voiceNote[v]/127.f);
        for (int i = 0; i < numSamples; i++)
        {
            float dummy = 0.f;
            tFiltModule_tick (state_.params.modules[v], &dummy);
        }

    }
}
