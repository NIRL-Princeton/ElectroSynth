//
// Created by Davis Polito on 6/5/25.
//

#include "ModuleList.h"
#include "synth_base.h"
#include "OscillatorModuleProcessor.h"
#include "FilterModuleProcessor.h"
#include "StringModuleProcessor.h"
#include <juce_core/juce_core.h>
#include "Modulators/EnvModuleProcessor.h"
#include "Modulators/LFOModuleProcessor.h"
template<typename T>
ModuleList<T>::ModuleList(SynthBase *synth) : tracktion::engine::ValueTreeObjectList<T>(synth->tree),synth_(synth){
    if constexpr (std::is_same_v<T, ProcessorBase>)
    {
       factory.template registerType<OscillatorModuleProcessor,electrosynth::SoundEngine*, juce::ValueTree, LEAF*>("osc");
       factory.template registerType<FilterModuleProcessor, electrosynth::SoundEngine*,juce::ValueTree, LEAF*>("filt");
        factory.template registerType<StringModuleProcessor,electrosynth::SoundEngine*, juce::ValueTree, LEAF*>("string");
    }
    else if constexpr (std::is_same_v<T, ModulatorBase>)
    {

        factory.template registerType<EnvModuleProcessor, electrosynth::SoundEngine*,juce::ValueTree, LEAF*>("env");
        factory.template registerType<LFOModuleProcessor, electrosynth::SoundEngine*,juce::ValueTree, LEAF*>("lfo");
    }
}

template<typename T>
ModuleList<T>::~ModuleList() {
    tracktion::ValueTreeObjectList<T>::freeObjects();
}
template<typename T>
void ModuleList<T>::deleteObject(T* processor_base) {

}
template<typename T>
T* ModuleList<T>::createNewObject(const juce::ValueTree& v) {
    auto* leaf  = synth_->getLeaf();
    std::any args = std::make_tuple(synth_->getEngine(),v,leaf);
    try {
        auto proc = factory.create(v.getProperty(IDs::type).toString().toStdString(),args);
        if constexpr (std::is_same_v<T, ProcessorBase>)
        {
            synth_->processorInitQueue.try_enqueue([this, proc] {
                synth_->addProcessor(proc, 0);
            });
        }
        else if constexpr (std::is_same_v<T, ModulatorBase>) {
            synth_->processorInitQueue.try_enqueue([this, proc] {
                 synth_->addModulationSource(proc, 0);
             });
        }
        modules_.push_back(proc);
        return proc.get();
    }catch (const std::bad_any_cast& e) {
        std::cerr << "Error during object creation: " << e.what() << std::endl;
    }
    return nullptr;
}
template<typename T>
void ModuleList<T>::newObjectAdded(T* processor) {
    for (auto listener: listeners_) {
        listener->moduleAdded(processor);
    }
}

//explicitly define the types thus telling the compiler to generate their code

template class ModuleList<ProcessorBase>;
template class ModuleList<ModulatorBase>;