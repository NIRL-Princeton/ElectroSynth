#include "FxDragCoordinator.h"

#include "EffectsModuleSection.h"
#include "ModuleSection.h"

FxDragCoordinator::FxDragCoordinator(juce::Component& sharedContentClip)
    : sharedContentClip_(sharedContentClip) {
    juce::Desktop::getInstance().addGlobalMouseListener(this);
}

FxDragCoordinator::~FxDragCoordinator() {
    cancelDrag();
    for (auto& registered : lanes_) {
        registered.lane->removeKeyListener(this);
        registered.lane->setDragCoordinator(nullptr);
    }
    juce::Desktop::getInstance().removeGlobalMouseListener(this);
}

void FxDragCoordinator::registerLane(EffectModuleSection& lane, LaneIdentifier identifier) {
    lanes_.push_back({ &lane, std::move(identifier) });
    lane.setDragCoordinator(this);
    lane.addKeyListener(this);
}

void FxDragCoordinator::unregisterLane(EffectModuleSection& lane) {
    if (phase_ != Phase::Idle)
        cancelDrag();
    lane.removeKeyListener(this);
    lane.setDragCoordinator(nullptr);
    std::erase_if(lanes_, [&lane](const RegisteredLane& registered) {
        return registered.lane == &lane;
    });
}

void FxDragCoordinator::dragStarted(EffectModuleSection& source, ModuleSection& module,
                                    juce::Point<int> mouseDownScreen) {
    if (!isDevelopmentGateEnabled())
        return;

    if (phase_ != Phase::Idle)
        cancelDrag();

    draggedModule_ = &module;
    sourceLane_ = &source;
    traversalLane_ = &source;
    mouseDownScreen_ = mouseDownScreen;
    currentPointerScreen_ = mouseDownScreen;
    traversalAnchorScreen_ = mouseDownScreen;
    pointerOffsetInModule_ = mouseDownScreen - module.getScreenBounds().getPosition();
    phase_ = Phase::SameLaneDrag;
    horizontalDirection_ = 0;
    targetInsertionIndex_ = -1;
    hasEnteredDestination_ = false;
    targetVerticalMode_ = false;
    startTimerHz(30);
}

void FxDragCoordinator::dragMoved(ModuleSection& module, juce::Point<int> pointerScreen) {
    if (!ownsDrag(&module))
        return;

    currentPointerScreen_ = pointerScreen;
    updateHorizontalIntent(pointerScreen);

    if (armedLane_ != nullptr) {
        if (hasEnteredDestination_ && targetVerticalMode_)
            updateTargetPreview(pointerScreen);
        else if (hasEnteredDestination_)
            updateTargetHover(pointerScreen);
        else
            updateArmedPreview(pointerScreen);

        if (hostedMidpointIsInsideArmedLane())
            enterArmedLane(pointerScreen);
    }

    if (hasEnteredDestination_ && armedLane_ == nullptr) {
        if (targetVerticalMode_)
            updateTargetPreview(pointerScreen);
        else
            updateTargetHover(pointerScreen);
    }
}

void FxDragCoordinator::dragEnded(ModuleSection& module, juce::Point<int> pointerScreen) {
    if (!ownsDrag(&module))
        return;

    const bool valid_target = hasEnteredDestination_ && enteredTargetLane_ != nullptr
                           && enteredTargetLane_->getContentViewportScreenBounds().contains(pointerScreen);
    finish(valid_target, pointerScreen);
}

void FxDragCoordinator::cancelDrag() {
    if (phase_ != Phase::Idle)
        finish(false, currentPointerScreen_);
}

bool FxDragCoordinator::ownsDrag(const ModuleSection* module) const noexcept {
    return phase_ != Phase::Idle && draggedModule_ == module;
}

bool FxDragCoordinator::isExternalPreviewActive(const ModuleSection* module) const noexcept {
    return ownsDrag(module)
        && externallyHostedModule_.load(std::memory_order_acquire) == module;
}

void FxDragCoordinator::mouseDrag(const juce::MouseEvent& event) {
    if (draggedModule_ != nullptr)
        dragMoved(*draggedModule_, event.source.getScreenPosition().roundToInt());
}

void FxDragCoordinator::mouseUp(const juce::MouseEvent& event) {
    if (draggedModule_ != nullptr)
        dragEnded(*draggedModule_, event.source.getScreenPosition().roundToInt());
}

bool FxDragCoordinator::keyPressed(const juce::KeyPress& key, juce::Component*) {
    if (phase_ != Phase::Idle && key == juce::KeyPress::escapeKey) {
        cancelDrag();
        return true;
    }
    return false;
}

void FxDragCoordinator::timerCallback() {
    if (phase_ == Phase::Idle)
        return;

    if (!juce::ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown()
        || (draggedModule_ != nullptr
            && draggedModule_->isCurrentlyBlockedByAnotherModalComponent())) {
        // A normal release is observed by the global mouse listener. Reaching this
        // path means capture/release delivery was lost, so restore without emitting.
        cancelDrag();
        return;
    }

    const auto pointer = juce::Desktop::getInstance().getMainMouseSource().getScreenPosition().roundToInt();
    if (pointer != currentPointerScreen_)
        dragMoved(*draggedModule_, pointer);
    else if (hasEnteredDestination_) {
        if (targetVerticalMode_)
            updateTargetPreview(pointer);
        else
            updateTargetHover(pointer);
    }
}

int FxDragCoordinator::laneIndex(const EffectModuleSection* lane) const {
    for (int i = 0; i < static_cast<int>(lanes_.size()); ++i)
        if (lanes_[i].lane == lane)
            return i;
    return -1;
}

EffectModuleSection* FxDragCoordinator::adjacentLane(EffectModuleSection* lane, int direction) const {
    const int index = laneIndex(lane);
    const int adjacent = index + direction;
    if (index < 0 || adjacent < 0 || adjacent >= static_cast<int>(lanes_.size()))
        return nullptr;
    return lanes_[adjacent].lane;
}

LaneIdentifier FxDragCoordinator::identifierFor(const EffectModuleSection* lane) const {
    const int index = laneIndex(lane);
    return index >= 0 ? lanes_[index].identifier : LaneIdentifier {};
}

void FxDragCoordinator::updateHorizontalIntent(juce::Point<int> pointerScreen) {
    if (sourceLane_ == nullptr || traversalLane_ == nullptr)
        return;

    if (hasEnteredDestination_) {
        if (armedLane_ != nullptr
            && traversalLane_->getContentViewportScreenBounds().contains(pointerScreen)) {
            const int directional_distance = horizontalDirection_
                                           * (pointerScreen.x - traversalAnchorScreen_.x);
            if (directional_distance
                < kHorizontalActivationDistance - kHorizontalIntentHysteresis) {
                clearArmedLane();
                phase_ = targetVerticalMode_ ? Phase::TargetLanePreview
                                             : Phase::TargetLaneHover;
            }
        }
        return;
    }

    if (armedLane_ != nullptr) {
        const int directional_distance = horizontalDirection_
                                       * (pointerScreen.x - traversalAnchorScreen_.x);
        if (directional_distance
            < kHorizontalActivationDistance - kHorizontalIntentHysteresis) {
            clearArmedLane();
            externallyHostedModule_.store(nullptr, std::memory_order_release);
            sourceLane_->restoreExternalVisualHosting(*draggedModule_);
            phase_ = Phase::SameLaneDrag;
            horizontalDirection_ = 0;
        }
        return;
    }

    const auto delta = pointerScreen - traversalAnchorScreen_;
    const int abs_x = std::abs(delta.x);
    const int abs_y = std::abs(delta.y);
    if (abs_x < kHorizontalActivationDistance
        || static_cast<float>(abs_x) < static_cast<float>(abs_y) * kHorizontalOverVerticalDominance) {
        // Vertical reorder remains the neutral mode. Refreshing the anchor after a
        // clearly vertical step lets a later deliberate horizontal gesture activate
        // independently of all vertical travel accumulated earlier in the drag.
        if (abs_y >= kHorizontalActivationDistance && abs_y > abs_x)
            traversalAnchorScreen_ = pointerScreen;
        return;
    }

    const int direction = delta.x < 0 ? -1 : 1;
    if (auto* adjacent = adjacentLane(traversalLane_, direction))
        armLane(*adjacent, direction);
}

void FxDragCoordinator::armLane(EffectModuleSection& lane, int direction) {
    clearArmedLane();
    armedLane_ = &lane;
    horizontalDirection_ = direction;

    if (externallyHostedModule_.load(std::memory_order_acquire) == nullptr
        && sourceLane_ != nullptr && draggedModule_ != nullptr) {
        sourceLane_->beginExternalVisualHosting(*draggedModule_, sharedContentClip_);
        externallyHostedModule_.store(draggedModule_, std::memory_order_release);
    }

    for (auto& registered : lanes_) {
        registered.lane->setExternalTransferHighlight(registered.lane == armedLane_);
        registered.lane->setExternalTransferDimmed(registered.lane != armedLane_);
    }
    phase_ = Phase::AdjacentLaneArmed;
}

void FxDragCoordinator::updateArmedPreview(juce::Point<int> pointerScreen) {
    if (draggedModule_ == nullptr || sourceLane_ == nullptr || traversalLane_ == nullptr)
        return;

    const auto hosted_screen = traversalLane_->getExternalHostedModuleScreenBounds(
        draggedModule_->getHeight(), pointerScreen.y, pointerOffsetInModule_.y,
        pointerScreen.x - traversalAnchorScreen_.x);
    sourceLane_->updateExternalHostedModuleBounds(*draggedModule_, hosted_screen,
                                                  sharedContentClip_);
}

bool FxDragCoordinator::hostedMidpointIsInsideArmedLane() const {
    if (draggedModule_ == nullptr || armedLane_ == nullptr)
        return false;

    const int midpoint_x = draggedModule_->getScreenBounds().getCentreX();
    const auto target = armedLane_->getContentViewportScreenBounds();
    return midpoint_x >= target.getX() && midpoint_x < target.getRight();
}

void FxDragCoordinator::enterArmedLane(juce::Point<int> pointerScreen) {
    if (armedLane_ == nullptr || draggedModule_ == nullptr || sourceLane_ == nullptr)
        return;

    auto* new_target = armedLane_;
    clearArmedLane();

    if (!hasEnteredDestination_) {
        sourceLane_->excludeExternalSourceFromLayout(*draggedModule_);
        hasEnteredDestination_ = true;
    }

    if (enteredTargetLane_ != nullptr && enteredTargetLane_ != new_target)
        enteredTargetLane_->clearExternalTargetPreview();

    enteredTargetLane_ = new_target;
    traversalLane_ = new_target;
    // Consume the horizontal gesture that produced this entry: every intent anchor
    // restarts here so accumulated pre-entry travel cannot leak into hover decisions.
    traversalAnchorScreen_ = pointerScreen;
    verticalIntentAnchorScreen_ = pointerScreen;
    lastTargetHoverPointerScreen_ = pointerScreen;
    targetVerticalMode_ = false;
    phase_ = Phase::TargetLaneHover;
    targetInsertionIndex_ = enteredTargetLane_->beginExternalTargetPreview(
        *draggedModule_, pointerScreen);
}

void FxDragCoordinator::updateTargetHover(juce::Point<int> pointerScreen) {
    if (draggedModule_ == nullptr || sourceLane_ == nullptr || enteredTargetLane_ == nullptr)
        return;

    auto hosted_screen = draggedModule_->getScreenBounds();
    hosted_screen.setPosition(pointerScreen.x - pointerOffsetInModule_.x,
                              pointerScreen.y - pointerOffsetInModule_.y);
    sourceLane_->updateExternalHostedModuleBounds(*draggedModule_, hosted_screen,
                                                  sharedContentClip_);
    targetInsertionIndex_ = enteredTargetLane_->updateExternalTargetHover(
        *draggedModule_, pointerScreen);

    if (armedLane_ != nullptr)
        return;

    // Rolling vertical-intent anchor: any horizontally dominated step re-anchors at
    // the pointer, so vertical intent is judged only against recent post-entry
    // direction rather than every horizontal pixel accumulated since lane entry.
    const auto step = pointerScreen - lastTargetHoverPointerScreen_;
    lastTargetHoverPointerScreen_ = pointerScreen;
    if (std::abs(step.x) > std::abs(step.y))
        verticalIntentAnchorScreen_ = pointerScreen;

    const auto delta = pointerScreen - verticalIntentAnchorScreen_;
    const int abs_x = std::abs(delta.x);
    const int abs_y = std::abs(delta.y);
    const int vertical_activation_distance = std::max(
        1, enteredTargetLane_->getContentViewportScreenBounds().getHeight()
             / kVerticalActivationLaneDivisor);
    if (abs_y >= vertical_activation_distance
        && static_cast<float>(abs_y)
             >= static_cast<float>(abs_x) * kVerticalOverHorizontalDominance) {
        enterTargetVerticalDrag(pointerScreen);
    }

    // Deliberately no armLane() here: the horizontal gesture that entered this lane
    // is consumed at entry, and a new adjacent lane may only be armed by a fresh
    // horizontal gesture after vertical mode is established (updateTargetPreview).
}

void FxDragCoordinator::enterTargetVerticalDrag(juce::Point<int> pointerScreen) {
    if (draggedModule_ == nullptr || sourceLane_ == nullptr || enteredTargetLane_ == nullptr)
        return;

    targetVerticalMode_ = true;
    phase_ = Phase::TargetLanePreview;
    traversalAnchorScreen_ = pointerScreen;

    const auto hosted_screen = enteredTargetLane_->getExternalHostedModuleScreenBounds(
        draggedModule_->getHeight(), pointerScreen.y, pointerOffsetInModule_.y, 0);
    sourceLane_->updateExternalHostedModuleBounds(*draggedModule_, hosted_screen,
                                                  sharedContentClip_);
    targetInsertionIndex_ = enteredTargetLane_->beginExternalTargetVerticalDrag(
        *draggedModule_, pointerScreen);
}

void FxDragCoordinator::updateTargetPreview(juce::Point<int> pointerScreen) {
    if (draggedModule_ == nullptr || enteredTargetLane_ == nullptr)
        return;

    // Match ordinary vertical reorder after entry: the exact source ModuleSection
    // visual is locked to the active lane's module column and follows pointer Y.
    const int horizontal_offset = phase_ == Phase::AdjacentLaneArmed
                                ? pointerScreen.x - traversalAnchorScreen_.x
                                : 0;
    const auto hosted_screen = enteredTargetLane_->getExternalHostedModuleScreenBounds(
        draggedModule_->getHeight(), pointerScreen.y, pointerOffsetInModule_.y,
        horizontal_offset);
    sourceLane_->updateExternalHostedModuleBounds(*draggedModule_, hosted_screen,
                                                  sharedContentClip_);

    // Keep the current preview alive while another adjacent lane is merely armed.
    targetInsertionIndex_ = enteredTargetLane_->updateExternalTargetVerticalDrag(
        *draggedModule_, pointerScreen);

    if (armedLane_ == nullptr) {
        const auto delta = pointerScreen - traversalAnchorScreen_;
        const int activation = kHorizontalActivationDistance;
        if (std::abs(delta.x) >= activation
            && static_cast<float>(std::abs(delta.x))
                   >= static_cast<float>(std::abs(delta.y)) * kHorizontalOverVerticalDominance) {
            const int direction = delta.x < 0 ? -1 : 1;
            if (auto* next = adjacentLane(traversalLane_, direction))
                armLane(*next, direction);
        }
        else if (std::abs(delta.y) >= activation && std::abs(delta.y) > std::abs(delta.x)) {
            traversalAnchorScreen_ = pointerScreen;
        }
    }
}

void FxDragCoordinator::finish(bool emitIntent, juce::Point<int> pointerScreen) {
    stopTimer();

    ModuleSection* module = draggedModule_;
    EffectModuleSection* source = sourceLane_;
    EffectModuleSection* target = enteredTargetLane_;
    const int insertion_index = targetInsertionIndex_;
    const bool entered = hasEnteredDestination_;

    clearArmedLane();
    if (target != nullptr)
        target->clearExternalTargetPreview();
    externallyHostedModule_.store(nullptr, std::memory_order_release);

    if (source != nullptr && module != nullptr) {
        if (entered)
            source->restoreExternalSourcePreview(*module);
        else {
            if (module->getParentComponent() == &sharedContentClip_)
                source->restoreExternalVisualHosting(*module);
            source->finishSameLaneDrag(*module, true);
        }
        module->resetDragObservation();
    }

    phase_ = Phase::Idle;
    draggedModule_ = nullptr;
    sourceLane_ = nullptr;
    traversalLane_ = nullptr;
    armedLane_ = nullptr;
    enteredTargetLane_ = nullptr;
    horizontalDirection_ = 0;
    targetInsertionIndex_ = -1;
    hasEnteredDestination_ = false;
    targetVerticalMode_ = false;

    if (emitIntent && module != nullptr && source != nullptr && target != nullptr
        && target != source
        && insertion_index >= 0 && onMoveRequested) {
        onMoveRequested({ module->getAudioNodeId(), identifierFor(source),
                          identifierFor(target), insertion_index });
    }

    juce::ignoreUnused(pointerScreen);
}

void FxDragCoordinator::clearArmedLane() {
    for (auto& registered : lanes_) {
        registered.lane->setExternalTransferHighlight(false);
        registered.lane->setExternalTransferDimmed(false);
    }
    armedLane_ = nullptr;
}
