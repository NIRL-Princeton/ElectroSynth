#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

class EffectModuleSection;
class ModuleSection;

struct LaneIdentifier {
    juce::String value;

    bool operator==(const LaneIdentifier&) const = default;
};

struct FxMoveIntent {
    juce::String moduleAudioNodeId;
    LaneIdentifier sourceLane;
    LaneIdentifier targetLane;
    int targetEffectIndex = -1;
};

class FxDragCoordinator final : private juce::MouseListener,
                                private juce::KeyListener,
                                private juce::Timer {
public:
    enum class Phase {
        Idle,
        SameLaneDrag,
        AdjacentLaneArmed,
        TargetLaneHover,
        TargetLanePreview
    };

    // Intentionally disabled in normal builds until FxMoveIntent can be accepted as
    // one state/ownership/DSP transaction.
    static constexpr bool isDevelopmentGateEnabled() noexcept {
#if defined(ELECTROSYNTH_ENABLE_INTER_LANE_FX_DRAG) && ELECTROSYNTH_ENABLE_INTER_LANE_FX_DRAG
        return true;
#elif !defined(ELECTROSYNTH_ENABLE_INTER_LANE_FX_DRAG) && JUCE_DEBUG
        // Debug standalones are the internal interaction test surface. Release builds
        // remain off unless an explicit development define is deliberately supplied.
        return true;
#else
        return false;
#endif
    }

    explicit FxDragCoordinator(juce::Component& sharedContentClip);
    ~FxDragCoordinator() override;

    void registerLane(EffectModuleSection& lane, LaneIdentifier identifier);
    void unregisterLane(EffectModuleSection& lane);
    void dragStarted(EffectModuleSection& source, ModuleSection& module,
                     juce::Point<int> mouseDownScreen);
    void dragMoved(ModuleSection& module, juce::Point<int> pointerScreen);
    void dragEnded(ModuleSection& module, juce::Point<int> pointerScreen);
    void cancelDrag();

    bool ownsDrag(const ModuleSection* module) const noexcept;
    bool isExternalPreviewActive(const ModuleSection* module) const noexcept;
    Phase getPhase() const noexcept { return phase_; }
    ModuleSection* getExternallyHostedModule() const noexcept {
        return externallyHostedModule_.load(std::memory_order_acquire);
    }

    // Returns true only when state, UI ownership, and DSP transfer were accepted
    // as one transaction. A rejected intent restores the source preview.
    std::function<bool(const FxMoveIntent&)> onMoveRequested;

private:
    struct RegisteredLane {
        EffectModuleSection* lane = nullptr;
        LaneIdentifier identifier;
    };

    static constexpr int kHorizontalActivationDistance = 7;
    static constexpr float kHorizontalOverVerticalDominance = 1.35f;
    static constexpr int kHorizontalIntentHysteresis = 3;
    static constexpr int kVerticalActivationLaneDivisor = 3;
    static constexpr float kVerticalOverHorizontalDominance = 1.35f;

    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component*) override;
    void timerCallback() override;

    int laneIndex(const EffectModuleSection* lane) const;
    EffectModuleSection* adjacentLane(EffectModuleSection* lane, int direction) const;
    LaneIdentifier identifierFor(const EffectModuleSection* lane) const;
    void updateHorizontalIntent(juce::Point<int> pointerScreen);
    void armLane(EffectModuleSection& lane, int direction);
    void updateArmedPreview(juce::Point<int> pointerScreen);
    bool hostedMidpointIsInsideArmedLane() const;
    void enterArmedLane(juce::Point<int> pointerScreen);
    void updateTargetHover(juce::Point<int> pointerScreen);
    void enterTargetVerticalDrag(juce::Point<int> pointerScreen);
    void updateTargetPreview(juce::Point<int> pointerScreen);
    bool maybeArmAdjacentLane(juce::Point<int> pointerScreen);
    void updateArmedLaneEmphasis();
    void finish(bool emitIntent, juce::Point<int> pointerScreen);
    void clearArmedLane();

    std::vector<RegisteredLane> lanes_;
    juce::Component& sharedContentClip_;
    std::atomic<ModuleSection*> externallyHostedModule_ { nullptr };
    ModuleSection* draggedModule_ = nullptr;
    EffectModuleSection* sourceLane_ = nullptr;
    EffectModuleSection* traversalLane_ = nullptr;
    EffectModuleSection* armedLane_ = nullptr;
    EffectModuleSection* enteredTargetLane_ = nullptr;
    juce::Point<int> mouseDownScreen_;
    juce::Point<int> currentPointerScreen_;
    juce::Point<int> traversalAnchorScreen_;
    juce::Point<int> verticalIntentAnchorScreen_;
    juce::Point<int> lastTargetHoverPointerScreen_;
    juce::Point<int> pointerOffsetInModule_;
    Phase phase_ = Phase::Idle;
    int horizontalDirection_ = 0;
    int targetInsertionIndex_ = -1;
    bool hasEnteredDestination_ = false;
    bool targetVerticalMode_ = false;
};
