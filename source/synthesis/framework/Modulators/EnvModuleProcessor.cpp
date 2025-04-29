//
// Created by Davis Polito on 11/19/24.
//

#include "EnvModuleProcessor.h"
EnvModuleProcessor::EnvModuleProcessor(juce::ValueTree& vt, LEAF* leaf)
    :ModulatorStateBase<PluginStateImpl_<EnvParamHolder, _tEnvModule>>(leaf,vt )
{
   proc = state.params.processors;
   vt.setProperty(IDs::uuid, state.params.processors[0].processorUniqueID, nullptr);
   name = vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString();
}

void EnvModuleProcessor::process()
{
    proc->tick(proc->object, nullptr);
}