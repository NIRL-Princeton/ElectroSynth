//
// Created by Davis Polito on 7/11/25.
//

#include "RoutingProcessor.h"
#include "ParameterView/RoutingView.h"
#include "mapping.h"
#include "sound_engine.h"

RoutingProcessor::RoutingProcessor(electrosynth::SoundEngine *engine, const juce::ValueTree &v, LEAF *leaf,juce::UndoManager * um) : ProcessorStateBase(engine,leaf,v,um){

        callbacks += {
                state_.addParameterListener (*state_.params.routing, chowdsp::ParameterListenerThread::AudioThread,
                    [this] {
                    auto routing = state_.params.routing.get();
                    float lane =  routing->getIndex(); /// (float)4; //numroutings
                    curr_lane = lane;
                        audio_out = &this->engine->temp_fx_buffers[curr_lane];
                    // for (auto mod: state_.params.modules) {
                    //     mod->setterFunctions[OscParams::OscType](mod,val);
                    //     mod->setterFunctions[OscParams::OscShapeParam](mod->theOsc, *mod->params[OscShapeParam]);
                    // //also need to update the shape since the new oscillator type will default to its initial shape instead
                    // }

                    // engine

                })
            };
    audio_out = &this->engine->temp_fx_buffers[0];
        state.setProperty(IDs::uuid, state_.params.processors[0].processorUniqueID, nullptr);
        name = state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString();
        procArray = &state_.params.processors[0];
}

std::unique_ptr<SynthSection> RoutingProcessor::createEditor() {


        return std::make_unique<RoutingView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());

}

void RoutingProcessor::processBlock(juce::AudioBuffer<float> & buffer, juce::MidiBuffer &) {
    state_.getParameterListeners().callAudioThreadBroadcasters();

    int numSamples = buffer.getNumSamples();
    //buffer.clear();

    //    auto* samplesL = buffer.getReadPointer(0);

    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        auto* inL = buffer.getWritePointer(v*2);
        auto* inR = buffer.getWritePointer(v * 2 + 1);

        auto* outL = audio_out->getWritePointer(v*2);
        auto* outR = audio_out->getWritePointer(v * 2 + 1);
        for (int i = 0; i < numSamples; i++)
        {
            procArray[v].tick(procArray[v].object,inL);
            outL[i] += procArray[v].outParameters[0];
            outR[i] = outL[i];
        }
    }
}
