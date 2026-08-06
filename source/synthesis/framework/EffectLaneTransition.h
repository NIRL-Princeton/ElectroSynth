#pragma once

#include <algorithm>
#include <cmath>

namespace electrosynth::effect_order {

class EffectLaneTransition {
public:
    static constexpr float kFadeDurationSeconds = 0.005f;

    enum class Phase {
        steady,
        fadingOut,
        silent,
        fadingIn
    };

    void setSampleRate(double sampleRate) noexcept {
        const auto safeSampleRate = std::max(1.0, sampleRate);
        fadeLengthSamples_ = std::max(1, static_cast<int>(
            std::lround(safeSampleRate * kFadeDurationSeconds)));
        gainStep_ = 1.0f / static_cast<float>(fadeLengthSamples_);
    }

    void beginFadeOut() noexcept {
        if (phase_ != Phase::silent)
            phase_ = Phase::fadingOut;
    }

    void beginFadeIn() noexcept {
        phase_ = gain_ >= 1.0f ? Phase::steady : Phase::fadingIn;
    }

    float advance() noexcept {
        if (phase_ == Phase::fadingOut) {
            gain_ = std::max(0.0f, gain_ - gainStep_);
            if (gain_ == 0.0f)
                phase_ = Phase::silent;
        } else if (phase_ == Phase::fadingIn) {
            gain_ = std::min(1.0f, gain_ + gainStep_);
            if (gain_ == 1.0f)
                phase_ = Phase::steady;
        }
        return gain_;
    }

    bool isSilent() const noexcept { return phase_ == Phase::silent; }
    float getGain() const noexcept { return gain_; }
    float getGainStep() const noexcept { return gainStep_; }
    int getFadeLengthSamples() const noexcept { return fadeLengthSamples_; }
    Phase getPhase() const noexcept { return phase_; }

private:
    Phase phase_ = Phase::steady;
    float gain_ = 1.0f;
    float gainStep_ = 1.0f / 220.0f;
    int fadeLengthSamples_ = 220;
};

} // namespace electrosynth::effect_order
