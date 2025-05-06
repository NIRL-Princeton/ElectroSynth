//
// Created by Davis Polito on 11/19/24.
//

#include "EnvModuleProcessor.h"

#include "sound_engine.h"

EnvModuleProcessor::EnvModuleProcessor(electrosynth::SoundEngine* engine,juce::ValueTree& vt, LEAF* leaf)
    :ModulatorStateBase<PluginStateImpl_<EnvParamHolder, _tEnvModule>>(engine,leaf,vt )
{
   procArray = &state.params.processors[0];
   vt.setProperty(IDs::uuid, state.params.processors[0].processorUniqueID, nullptr);
   name = vt.getProperty(IDs::type).toString() + vt.getProperty(IDs::uuid).toString();
}
