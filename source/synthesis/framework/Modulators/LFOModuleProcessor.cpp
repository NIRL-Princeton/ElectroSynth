//
// Created by Davis Polito on 1/27/25.
//

#include "LFOModuleProcessor.h"
LFOModuleProcessor::LFOModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree& vt, LEAF* leaf)
    :ModulatorStateBase(engine,leaf,vt )
{
    procArray = &state_.params.processors[0];
    vt.setProperty(IDs::uuid, state_.params.processors[0].processorUniqueID, nullptr);
    name = vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString();
}

void LFOModuleProcessor::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    //proc->tick(proc->object, nullptr);
}