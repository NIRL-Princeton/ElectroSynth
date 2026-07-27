//
// Created by Davis Polito on 7/10/25.
//

#ifndef EFFECTLIST_H
#define EFFECTLIST_H
#include "ModuleList.h"
#include "AudioNode.h"

class EffectList : public ModuleList<ProcessorBase> {

    public:
    EffectList(SynthBase* synth,const ValueTree&, int lane );
    void deleteObject(ProcessorBase *) override;
    bool isSuitableType(const juce::ValueTree &v) const override {
        return v.hasType(IDs::EFFECTMODULE);
    }

    ProcessorBase* createNewObject(const juce::ValueTree &) override;
    juce::String getAudioNodeId() const;
    electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const;
    const int lane;
};

#endif //EFFECTLIST_H
