//
// Created by Davis Polito on 1/27/25.
//

#include "LFOModuleProcessor.h"
LFOModuleProcessor::LFOModuleProcessor(juce::ValueTree& vt, LEAF* leaf)
    :ModulatorStateBase<PluginStateImpl_<LFOParamHolder, _tLFOModule>>(leaf,vt )
{
    proc = state.params.processors;
    vt.setProperty(IDs::uuid, state.params.processors[0].processorUniqueID, nullptr);
    name = vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString();
}

void LFOModuleProcessor::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    proc->tick(proc->object, nullptr);
}
void LFOModuleProcessor::process()
{
    proc->tick(proc->object, nullptr);
}