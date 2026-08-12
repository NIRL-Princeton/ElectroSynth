//
// Created by Matthew McWeeney on 7/9/26.
//

#include "NoiseModuleProcessor.h"
#include "sound_engine.h"

NoiseModuleProcessor::NoiseModuleProcessor(electrosynth::SoundEngine* engine,const juce::ValueTree &v, LEAF *leaf,juce::UndoManager* um) :ProcessorStateBase(engine,leaf,v,um)
{
    // callbacks += {
    //     state_.addParameterListener (*state_.params.peakBandwidth, chowdsp::ParameterListenerThread::AudioThread, [this] {
    //         auto thePeakBandwidth = state_.params.peakBandwidth.get();
    //         float val =  (float)thePeakBandwidth->getCurrentValue();
    //         for (auto mod: state_.params.modules) {
    //             tNoiseModule_setParameter(mod, NoisePeakBandwidth,val);
    //             tNoiseModule_setParameter(mod, NoisePeakFreq, *mod->header.params[NoisePeakFreq]);
    //         }
    //     })
    // };
}

// void NoiseModuleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
// {
//     state_.getParameterListeners().callAudioThreadBroadcasters();
//     int numSamples = buffer.getNumSamples();
//     //buffer.clear();
//
//     float glideOrigin = state_.params.modules[tStack_first(engine->voiceHandler.voiceOrder)]->pitchSmoother.curr;
//
//     int counter = 0;
//     for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
//
//         if (!engine->voiceHandler.voiceIsSounding[v])
//         {
//             counter++;
//             tNoiseModule_setGlideOrigin(state_.params.modules[v], glideOrigin);
//             continue;
//         }
//
//         tNoiseModule_setParameter(state_.params.modules[v], NoiseMIDIPitch,engine->voiceHandler.voiceNote[v]/127.f );
//
//         if (noVoicesSounding == 1 && state_.params.modules[v]->portaType < 0.5f)
//         {
//             printf("Woah glideO set! %f\n",glideOrigin);
//             tNoiseModule_setGlideOrigin(state_.params.modules[v], state_.params.modules[v]->pitchSmoother.dest);
//             noVoicesSounding = 0;
//         }
//
//         auto* L = buffer.getWritePointer(v*2);
//         auto* R = buffer.getWritePointer(v*2+1);
//         for (int i = 0; i < numSamples; i++)
//         {
//             tNoiseModule_tick(state_.params.modules[v],L);
//             L[i] += state_.params.modules[v]->header.outputs[0];
//             R[i] = L[i];
//         }
//     }
//
//     if (counter == 12)
//     {
//         noVoicesSounding = 1;
//     } else
//     {
//         noVoicesSounding = 0;
//     }
//     // ProcessorBase::processBlock(buffer,midi);
// }

void NoiseModuleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    state_.getParameterListeners().callAudioThreadBroadcasters();
    int numSamples = buffer.getNumSamples();
    //buffer.clear();

    float glideOrigin = state_.params.modules[tStack_first(engine->voiceHandler.voiceOrder)]->pitchSmoother.curr;

    //    auto* samplesL = buffer.getReadPointer(0);
    int counter = 0;
    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {

        if (!engine->voiceHandler.voiceIsSounding[v])
        {
            counter++;
            tNoiseModule_setGlideOrigin(state_.params.modules[v], glideOrigin);
            continue;
        }

        tNoiseModule_setParameter(state_.params.modules[v], NoiseMIDIPitch,engine->voiceHandler.voiceNote[v]/127.f );

        if (noVoicesSounding == 1 && state_.params.modules[v]->portaType < 0.5f)
        {
            tNoiseModule_setGlideOrigin(state_.params.modules[v], state_.params.modules[v]->pitchSmoother.dest);
            //state_.params.modules[v]->pitchSmooth.curr = state_.params.modules[v]->pitchSmooth.dest;
            noVoicesSounding = 0;
        }
        auto* L = buffer.getWritePointer(v*2);
        auto* R = buffer.getWritePointer(v*2+1);
        for (int i = 0; i < numSamples; i++)
        {
            tNoiseModule_tick(state_.params.modules[v],L);
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