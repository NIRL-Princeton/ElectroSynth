//
// Created by Davis Polito on 1/27/25.
//

#include "LFOModuleProcessor.h"
LFOModuleProcessor::LFOModuleProcessor(juce::ValueTree& vt, LEAF* leaf)
    :ModulatorStateBase<PluginStateImpl_<LFOParamHolder, _tLFOModule>>(leaf,vt )
{
    proc = &state.params.processor;
    vt.setProperty(IDs::uuid, state.params.processor.processorUniqueID, nullptr);
    name = vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString();
}