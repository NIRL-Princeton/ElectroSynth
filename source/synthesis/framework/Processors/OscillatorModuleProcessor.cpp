//
// Created by Davis Polito on 8/8/24.
//

#include "OscillatorModuleProcessor.h"
#include "Identifiers.h"
#include "sound_engine.h"
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
        return "1 / " + juce::String(abs(round(harmonic) - 1.f)) ;
    else
        return juce::String(round(harmonic + 1.f));
}




OscillatorModuleProcessor::OscillatorModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) :ProcessorStateBase(engine,leaf,v,um)


{
    // callbacks += {
    //     state_.addParameterListener (*state_.params.oscType, chowdsp::ParameterListenerThread::AudioThread, [this] {
    //         auto theType = state_.params.oscType.get();
    //         //float val =  (float)theType->getIndex() / (float)OscTypes::OscNumTypes;
    //         float val = theType->getCurrentValue();
    //         for (auto mod: state_.params.modules) {
    //             tOscModule_setParameter(mod, OscType,val);
    //             tOscModule_setParameter(mod, OscShapeParam, *mod->header.params[OscShapeParam]);
    //
    //         //also need to update the shape since the new oscillator type will default to its initial shape instead
    //         }
    //     })
    // };

   //tOscModule_init(static_cast<void*>(module), {0, 0}, id, leaf)
    //tOscModule_processorInit(state_.params.module, &processor);
    // tStack_create(&leaf->mempool, (tStack**)&activeModules);
    // tStack_init(leaf, &activeModules);
    // tStack_setCapacity(&activeModules, MAX_NUM_VOICES);

}

void OscillatorModuleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    state_.getParameterListeners().callAudioThreadBroadcasters();
    int numSamples = buffer.getNumSamples();
    //buffer.clear();

    // uint8_t leadVoice = tStack_first(engine->voiceHandler.voiceOrder);
    // if (leadVoice != leadingVoice && leadVoice <= 11)
    // {
    //     leadingVoice = leadVoice;
    //     tOscModule_setGlideOrigin(state_.params.modules[leadingVoice], state_.params.modules[leadingVoice]->pitchSmooth.curr);
    // }

    float glideOrigin = state_.params.modules[tStack_first(engine->voiceHandler.voiceOrder)]->pitchSmooth.curr;

    //    auto* samplesL = buffer.getReadPointer(0);
    int counter = 0;
    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {

        if (!engine->voiceHandler.voiceIsSounding[v])
        {
            counter++;
            tOscModule_setGlideOrigin(state_.params.modules[v], glideOrigin);
            continue;
        }

        tOscModule_setParameter(state_.params.modules[v], OscMidiPitch,engine->voiceHandler.voiceNote[v]/127.f );

        if (noVoicesSounding == 1 && state_.params.modules[v]->portaType < 0.5f)
        {
            tOscModule_setGlideOrigin(state_.params.modules[v], state_.params.modules[v]->pitchSmooth.dest);
            //state_.params.modules[v]->pitchSmooth.curr = state_.params.modules[v]->pitchSmooth.dest;
            noVoicesSounding = 0;
        }
        auto* L = buffer.getWritePointer(v*2);
        auto* R = buffer.getWritePointer(v*2+1);
        for (int i = 0; i < numSamples; i++)
        {
           tOscModule_tick(state_.params.modules[v],L);
            L[i] += state_.params.modules[v]->header.outputs[0];
            R[i] = L[i];
        }
    }

    if (counter == 12)
    {
        noVoicesSounding = 1;
    } else
    {
        noVoicesSounding = 0;
    }
    // ProcessorBase::processBlock(buffer,midi);
}
