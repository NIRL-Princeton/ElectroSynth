//
// Created by Davis Polito on 7/10/25.
//

#ifndef EFFECTLIST_H
#define EFFECTLIST_H
#include "ModuleList.h"
#include "Node.h"
#include <functional>

class EffectList : public ModuleList<ProcessorBase> {

public:
    EffectList(SynthBase* synth,const ValueTree&, int lane );
    ~EffectList() override;
    void deleteObject(ProcessorBase *) override;
    bool isSuitableType(const juce::ValueTree &v) const override {
        return v.hasType(IDs::EFFECTMODULE);
    }

    ProcessorBase* createNewObject(const juce::ValueTree &) override;
    void newObjectAdded(ProcessorBase*) override;
    void objectOrderChanged() override;
    void valueTreeChildOrderChanged(juce::ValueTree& tree, int oldIndex, int newIndex) override;
    void publishCurrentOrder();
    bool moveEffectTo(EffectList& target, const juce::String& moduleAudioNodeId,
                      int targetEffectIndex, juce::UndoManager& undoManager);
    bool transferEffectTo(EffectList& target, const juce::String& moduleAudioNodeId,
                          int targetEffectIndex);
    std::function<bool(ProcessorBase*, EffectList&, int)> onUiTransferRequested;
    electrosynth::audio::NodeDescriptor getAudioNodeDescriptor() const;
    juce::String getNodeId() const;
    const int lane;

private:
    ProcessorBase* findEffect(const juce::String& moduleAudioNodeId) const;
    int indexOfEffect(const ProcessorBase* processor) const;

    int movedChildNewIndex_ = -1;
    ProcessorBase* transferringOutProcessor_ = nullptr;
    ProcessorBase* transferringInProcessor_ = nullptr;
    juce::ValueTree transferringInState_;
};

#endif //EFFECTLIST_H
