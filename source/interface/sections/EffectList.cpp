//
// Created by Davis Polito on 7/10/25.
//
#include "EffectList.h"
#include "synth_base.h"
#include <algorithm>
#include <iterator>

namespace {
class EffectTransferAction final : public juce::UndoableAction {
public:
    EffectTransferAction(EffectList& source, EffectList& target,
                         juce::String audioNodeId, int sourceIndex, int targetIndex)
        : source_(source), target_(target), audioNodeId_(std::move(audioNodeId)),
          sourceIndex_(sourceIndex), targetIndex_(targetIndex) {}

    bool perform() override {
        return source_.transferEffectTo(target_, audioNodeId_, targetIndex_);
    }

    bool undo() override {
        return target_.transferEffectTo(source_, audioNodeId_, sourceIndex_);
    }

    int getSizeInUnits() override { return 1; }

private:
    EffectList& source_;
    EffectList& target_;
    juce::String audioNodeId_;
    int sourceIndex_ = -1;
    int targetIndex_ = -1;
};
} // namespace

EffectList::EffectList(SynthBase *synth, const ValueTree &v, int _lane) : ModuleList<ProcessorBase>(synth,v), lane(_lane) {
    synth_->registerEffectList(this);
}

EffectList::~EffectList() {
    synth_->unregisterEffectList(this);
}

void EffectList::valueTreeChildOrderChanged(juce::ValueTree& tree, int oldIndex, int newIndex) {
    if (tree == this->parent)
        movedChildNewIndex_ = newIndex;

    tracktion::engine::ValueTreeObjectList<ProcessorBase>::valueTreeChildOrderChanged(tree, oldIndex, newIndex);
    movedChildNewIndex_ = -1;
}

void EffectList::objectOrderChanged() {
    if (transferringOutProcessor_ != nullptr || transferringInProcessor_ != nullptr)
        return;

    ModuleList<ProcessorBase>::objectOrderChanged();

    if (!juce::isPositiveAndBelow(movedChildNewIndex_, this->parent.getNumChildren())) {
        publishCurrentOrder();
        return;
    }

    const auto movedState = this->parent.getChild(movedChildNewIndex_);
    const auto movedObject = std::find_if(this->objects.begin(), this->objects.end(),
                                          [&movedState](const auto* object) {
                                              return object != nullptr && object->state == movedState;
                                          });

    if (movedObject == this->objects.end()) {
        publishCurrentOrder();
        return;
    }

    const auto followingObject = std::next(movedObject);
    auto* nextObject = followingObject != this->objects.end() ? *followingObject : nullptr;
    synth_->submitEffectOrder(lane, *movedObject, nextObject);
}

void EffectList::publishCurrentOrder() {
    ProcessorBase* nextObject = nullptr;
    for (int index = this->objects.size(); --index >= 0;) {
        auto* object = this->objects[index];
        if (object != nullptr) {
            synth_->submitEffectOrder(lane, object, nextObject);
            nextObject = object;
        }
    }
}

void EffectList::deleteObject(ProcessorBase *base) {
    if (base == transferringOutProcessor_)
        return;

    synth_->removeEffect(base, lane);
    for (auto listener : listeners_)
        listener->removeModule(base);
}
ProcessorBase *EffectList::createNewObject(const juce::ValueTree &v) {
    if (transferringInProcessor_ != nullptr && v == transferringInState_)
        return transferringInProcessor_;

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

void EffectList::newObjectAdded(ProcessorBase* processor) {
    if (processor == transferringInProcessor_)
        return;
    ModuleList<ProcessorBase>::newObjectAdded(processor);
}

ProcessorBase* EffectList::findEffect(const juce::String& moduleAudioNodeId) const {
    const auto found = std::find_if(this->objects.begin(), this->objects.end(),
                                    [&moduleAudioNodeId](const auto* processor) {
                                        return processor != nullptr
                                            && processor->state.getProperty(IDs::audioNodeId).toString()
                                                   == moduleAudioNodeId;
                                    });
    return found == this->objects.end() ? nullptr : *found;
}

int EffectList::indexOfEffect(const ProcessorBase* processor) const {
    const auto found = std::find(this->objects.begin(), this->objects.end(), processor);
    return found == this->objects.end()
             ? -1
             : static_cast<int>(std::distance(this->objects.begin(), found));
}

bool EffectList::moveEffectTo(EffectList& target, const juce::String& moduleAudioNodeId,
                              int targetEffectIndex, juce::UndoManager& undoManager) {
    auto* processor = findEffect(moduleAudioNodeId);
    const int sourceIndex = indexOfEffect(processor);
    if (processor == nullptr || sourceIndex < 0 || &target == this)
        return false;

    return undoManager.perform(
        new EffectTransferAction(*this, target, moduleAudioNodeId, sourceIndex,
                                 targetEffectIndex),
        "Move effect between lanes");
}

bool EffectList::transferEffectTo(EffectList& target,
                                  const juce::String& moduleAudioNodeId,
                                  int targetEffectIndex) {
    auto* processor = findEffect(moduleAudioNodeId);
    if (processor == nullptr || &target == this || !onUiTransferRequested)
        return false;

    const int sourceIndex = indexOfEffect(processor);
    const auto processorState = processor->state;
    if (sourceIndex < 0 || !processorState.isValid()
        || processorState.getParent() != this->parent)
        return false;

    const int insertionIndex = juce::jlimit(0, target.parent.getNumChildren(),
                                             targetEffectIndex);
    auto* nextTargetProcessor = juce::isPositiveAndBelow(
                                    insertionIndex,
                                    static_cast<int>(target.objects.size()))
                                  ? target.objects[static_cast<std::size_t>(insertionIndex)]
                                  : nullptr;

    transferringOutProcessor_ = processor;
    target.transferringInProcessor_ = processor;
    target.transferringInState_ = processorState;
    this->parent.removeChild(processorState, nullptr);
    target.parent.addChild(processorState, insertionIndex, nullptr);
    transferringOutProcessor_ = nullptr;
    target.transferringInProcessor_ = nullptr;
    target.transferringInState_ = {};

    if (processorState.getParent() != target.parent
        || !onUiTransferRequested(processor, target, insertionIndex)) {
        target.transferringOutProcessor_ = processor;
        transferringInProcessor_ = processor;
        transferringInState_ = processorState;
        target.parent.removeChild(processorState, nullptr);
        this->parent.addChild(processorState, sourceIndex, nullptr);
        target.transferringOutProcessor_ = nullptr;
        transferringInProcessor_ = nullptr;
        transferringInState_ = {};
        return false;
    }

    synth_->submitEffectMove(lane, target.lane, processor, nextTargetProcessor);
    return true;
}
