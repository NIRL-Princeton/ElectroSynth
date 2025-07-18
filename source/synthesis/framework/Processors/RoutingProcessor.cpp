//
// Created by Davis Polito on 7/11/25.
//

#include "RoutingProcessor.h"
#include "ParameterView/RoutingView.h"
#include "mapping.h"
#include "sound_engine.h"

RoutingProcessor::RoutingProcessor(electrosynth::SoundEngine *engine, const juce::ValueTree &v, LEAF *leaf) : ProcessorStateBase(engine,leaf,v){

        callbacks += {
                state_.addParameterListener (*state_.params.routing, chowdsp::ParameterListenerThread::AudioThread,
                    [this] {
                    auto routing = state_.params.routing.get();
                    float lane =  (float)routing->getIndex() / (float)4; //numroutings
                    curr_lane = lane;
                    // for (auto mod: state_.params.modules) {
                    //     mod->setterFunctions[OscParams::OscType](mod,val);
                    //     mod->setterFunctions[OscParams::OscShapeParam](mod->theOsc, *mod->params[OscShapeParam]);
                    // //also need to update the shape since the new oscillator type will default to its initial shape instead
                    // }

                    // engine

                })
            };
        state.setProperty(IDs::uuid, state_.params.processors[0].processorUniqueID, nullptr);
        name = state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString();
        procArray = &state_.params.processors[0];
}

std::unique_ptr<SynthSection> RoutingProcessor::createEditor() {


        return std::make_unique<RoutingView>(state_, state_.params, state.getProperty(IDs::type).toString() + state.getProperty(IDs::uuid).toString());

}

void RoutingProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &) {
    int numSamples = buffer.getNumSamples();
    //buffer.clear();

    //    auto* samplesL = buffer.getReadPointer(0);
    for (int v = 0; v < engine->voiceHandler.numVoicesActive; v++) {
        auto* L = buffer.getWritePointer(v*2);
        auto* R = buffer.getWritePointer(v * 2 + 1);
        for (int i = 0; i < numSamples; i++)
        {
            procArray[v].tick(procArray[v].object,L);
            L[i] += procArray[v].outParameters[0];
            R[i] = L[i];
        }
    }
}
