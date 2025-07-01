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
ModuleList<T>::ModuleList(SynthBase *synth,const ValueTree& v) : tracktion::engine::ValueTreeObjectList<T>(v),synth_(synth),state(v){
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
    tracktion::engine::ValueTreeObjectList<T>::rebuildObjects();
    for (auto obj: tracktion::engine::ValueTreeObjectList<T>::objects) {
        newObjectAdded(obj);
    }
}

template<typename T>
ModuleList<T>::~ModuleList() {

    tracktion::ValueTreeObjectList<T>::freeObjects();
}
template<typename T>
void ModuleList<T>::deleteObject(T* processor_base) {
    DBG("deleteObject");

    synth_->removeProcessor(processor_base);
}

template <typename T>
void ModuleList<T>::objectRemoved(T* processor_base) {
    for (auto listener: listeners_) {
        listener->removeModule(processor_base);
    }

}
template<typename T>
T* ModuleList<T>::createNewObject(const juce::ValueTree& v) {
    auto* leaf  = synth_->getLeaf();
    std::any args = std::make_tuple(synth_->getEngine(),v,leaf);
    try {
        auto proc = factory.create(v.getProperty(IDs::type).toString().toStdString(),args);
        T* rawPtr = proc.get();
        assert(rawPtr != nullptr);
        if constexpr (std::is_same_v<T, ProcessorBase>)
        {
            auto task = [this, _proc = std::move(proc), index = v.getParent().getParent().indexOf(v.getParent())]() mutable {
                synth_->addProcessor(std::move(_proc), index);
            };

            synth_->processorInitQueue.try_enqueue(std::move(task));
        }
        else if constexpr (std::is_same_v<T, ModulatorBase>) {
            auto task = [this, _proc = std::move(proc)]() mutable {
                synth_->addModulationSource(std::move(_proc), 0);
            };
            synth_->processorInitQueue.try_enqueue(std::move(task));
        }

        return rawPtr;
    }catch (const std::bad_any_cast& e) {
        std::cerr << "Error during object creation: " << e.what() << std::endl;
        jassertfalse;
    }
    return nullptr;
}
template<typename T>
void ModuleList<T>::newObjectAdded(T* processor) {
    for (auto listener: listeners_) {
        listener->moduleAdded(processor);
    }
}
template<typename T>
void ModuleList<T>::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i) {

        tracktion::engine::ValueTreeObjectList<T>::valueTreePropertyChanged (v, i);
        if(v.getProperty(IDs::sync,0))
        {
            for(auto obj : tracktion::engine::ValueTreeObjectList<T>::objects)
            {
                juce::MemoryBlock data;
                obj->getStateInformation(data);
                auto xml = juce::parseXML(data.toString());
                //auto xml = juce::AudioProcessor::getXmlF(data.getData(), (int)data.getSize());
                if (obj->state.isValid() && xml != nullptr) {
                    auto uuid = obj->state.getProperty(IDs::uuid).toString();
                    auto type = obj->state.getProperty(IDs::type).toString();
                    obj->state.copyPropertiesFrom(juce::ValueTree::fromXml(*xml),nullptr);
                    obj->state.setProperty(IDs::type, type,nullptr);
                    obj->state.setProperty(IDs::uuid, uuid,nullptr);
                    //  state.addChild(juce::ValueTree::fromXml(*xml),0,nullptr);
                }
            }
            v.removeProperty(IDs::sync, nullptr);
        }

}


template<typename T>
ChainList<T>::ChainList(SynthBase* synth, const juce::ValueTree& v) : tracktion::engine::ValueTreeObjectList<ModuleList<T>>(v), synth_(synth){
    tracktion::engine::ValueTreeObjectList<ModuleList<T>>::rebuildObjects();
}

template<typename T>
ModuleList<T> *ChainList<T>::createNewObject(const juce::ValueTree &v) {
    auto index = v.getParent().indexOf(v);
    ModuleList<T> *list = new ModuleList<T>(synth_,v);
    return list;

}
template<typename T>
void ChainList<T>::newObjectAdded ( ModuleList<T>*t) {
    for (auto listener: listeners_) {
        listener->chainAdded(t);
    }
}

template<typename T>
ChainList<T>::~ChainList() {

    tracktion::ValueTreeObjectList<ModuleList<T>>::freeObjects();
}
template<typename T>
void ChainList<T>::deleteObject(ModuleList<T>* processor_base) {
    DBG("deleteObject");
    processor_base->freeObjects();

}

template <typename T>
void ChainList<T>::objectRemoved(ModuleList<T>* processor_base) {
    for (auto listener: listeners_) {
        listener->removeChain(processor_base);
    }

}
//explicitly define the types thus telling the compiler to generate their code

template class ModuleList<ProcessorBase>;
template class ModuleList<ModulatorBase>;
template class ChainList<ProcessorBase>;