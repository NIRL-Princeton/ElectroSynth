//
// Created by Davis Polito on 8/8/24.
//

#include "OscillatorModuleProcessor.h"
#include "Identifiers.h"
float electrosynth::utils::stringToHarmonicVal(const juce::String &s){
    if(!s.contains("/"))
    {
        return s.getFloatValue();
    }
    else
    {
        juce::StringArray tokens;
        tokens.addTokens(s,"/","\"");
        return tokens[1].getFloatValue();
    }
}

juce::String electrosynth::utils::harmonicValToString(float harmonic)
{
    if(harmonic < 0.f)
        return "1 / " + juce::String(abs(round(harmonic)));
    else
        return juce::String(round(harmonic));
}
OscillatorModuleProcessor::OscillatorModuleProcessor(const juce::ValueTree &v, LEAF *leaf) :ProcessorStateBase<PluginStateImpl_<OscillatorParams, _tOscModule>>(leaf,v)


{

    callbacks += {
        state.addParameterListener (*state.params.oscType, chowdsp::ParameterListenerThread::AudioThread, [this] {
            auto theType = state.params.oscType.get();
            float val =  (float)theType->getIndex() / (float)OscTypes::OscNumTypes;
            for (auto mod: state.params.modules) {
                mod->setterFunctions[OscParams::OscType](mod,val);
                mod->setterFunctions[OscParams::OscShapeParam](mod->theOsc, *mod->params[OscShapeParam]);
            //also need to update the shape since the new oscillator type will default to its initial shape instead
            }



        })
    };
    vt.setProperty(IDs::uuid, state.params.processors[0].processorUniqueID, nullptr);
    name = vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString();
    procArray = &state.params.processors[0];

   //tOscModule_init(static_cast<void*>(module), {0, 0}, id, leaf)
    //tOscModule_processorInit(state.params.module, &processor);
}

void OscillatorModuleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    state.getParameterListeners().callAudioThreadBroadcasters();
    int numSamples = buffer.getNumSamples();
    //buffer.clear();

//    auto* samplesL = buffer.getReadPointer(0);
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    for (int i = 0; i < numSamples; i++)
    {

        tOscModule_tick(state.params.modules[0],L);
        L[i] += state.params.modules[0]->outputs[0];
        R[i] = L[i];
    }

}