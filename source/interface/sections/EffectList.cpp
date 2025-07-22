//
// Created by Davis Polito on 7/10/25.
//
#include "EffectList.h"
#include "synth_base.h"

EffectList::EffectList(SynthBase *synth, const ValueTree &v, int _lane) : ModuleList<ProcessorBase>(synth,v), lane(_lane) {

}

void EffectList::deleteObject(ProcessorBase *base) {
    synth_->removeEffect(base, lane);
}
ProcessorBase *EffectList::createNewObject(const juce::ValueTree &v) {
    auto* leaf  = synth_->getLeaf();
    std::any args = std::make_tuple(synth_->getEngine(),v,leaf,&synth_->um);
    try {
        auto proc = factory.create(v.getProperty(IDs::type).toString().toStdString(),args);
        ProcessorBase* rawPtr = proc.get();
        assert(rawPtr != nullptr);

            auto task = [this, _proc = std::move(proc), index = lane]() mutable {
                synth_->addEffect(std::move(_proc), index);
            };

            synth_->processorInitQueue.try_enqueue(std::move(task));



        return rawPtr;
    }catch (const std::bad_any_cast& e) {
        std::cerr << "Error during object creation: " << e.what() << std::endl;
        jassertfalse;
    }
    return nullptr;
}
