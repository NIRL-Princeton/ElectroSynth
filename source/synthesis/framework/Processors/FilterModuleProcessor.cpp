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
FilterModuleProcessor::FilterModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf) : ProcessorStateBase(engine,leaf,v)
{
   //tOscModule_init(static_cast<void*>(module), {0, 0}, id, leaf)
    //tFiltModule_processorInit(state_.params.module, &processor);
   state.setProperty(IDs::uuid, state_.params.processors[0].processorUniqueID, nullptr);
    procArray = &state_.params.processors[0];
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
        (state_.params.modules[0], L);

        R[i] = L[i];
    }

}