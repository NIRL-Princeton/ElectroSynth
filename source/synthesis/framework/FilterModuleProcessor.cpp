//
// Created by Davis Polito on 8/8/24.
//

#include "FilterModuleProcessor.h"
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
FilterModuleProcessor::FilterModuleProcessor(const juce::ValueTree &v, LEAF *leaf) :_PluginBase<PluginStateImpl_<FilterParams, _tFiltModule>, _tFiltModule>(leaf)


{
   //tOscModule_init(static_cast<void*>(module), {0, 0}, id, leaf)
    //tFiltModule_processorInit(state.params.module, &processor);
}

void FilterModuleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    int numSamples = buffer.getNumSamples();
    //buffer.clear();
//    auto* samplesL = buffer.getReadPointer(0);
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    for (int i = 0; i < numSamples; i++)
    {
        tFiltModule_tick(state.params.module, L);

        R[i] = L[i];
    }

}