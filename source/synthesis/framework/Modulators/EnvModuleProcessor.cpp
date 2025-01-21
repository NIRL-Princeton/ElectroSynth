//
// Created by Davis Polito on 11/19/24.
//

#include "EnvModuleProcessor.h"
EnvModuleProcessor::EnvModuleProcessor(juce::ValueTree& vt, LEAF* leaf)
    :ModulatorStateBase<PluginStateImpl_<EnvParamHolder, _tEnvModule>>(leaf,vt )
{
   proc = &state.params.processor;
   vt.setProperty(IDs::uuid, state.params.processor.processorUniqueID, nullptr);
   name = vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString();
}