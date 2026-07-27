//
// Created by Callista Chong on 7/25/26.
//

#pragma once
#include "audio_port_component.h"
#include "open_gl_multi_quad.h"
#include "synth_section.h"
#include <array>
#include <vector>

class AudioConnectionSlots final : public SynthSection {

public:
    static constexpr int kMaxVisibleSlots = 4;
    static constexpr int kSlotSize = 8;
    static constexpr int kSlotGap = 2;
    static constexpr int kSlotPitch = kSlotSize + kSlotGap;
    static constexpr int kPreferredWidth =
        kMaxVisibleSlots * kSlotSize + (kMaxVisibleSlots - 1) * kSlotGap;

    explicit AudioConnectionSlots(AudioPortComponent& port);
    ~AudioConnectionSlots() override;

    void setDestinations(
        std::vector<electrosynth::audio::AudioPortAddress> destinations);
    int getConnectionCount() const noexcept {
        return static_cast<int>(destinations_.size());
    }
    void resized() override;
    void paintBackground(Graphics&) override {}

private:
    struct SlotVisual {
        std::shared_ptr<OpenGlQuad> body;
        std::shared_ptr<OpenGlQuad> border;
    };

    void syncOpenGl();

    AudioPortComponent& port_;
    std::vector<electrosynth::audio::AudioPortAddress> destinations_;
    std::array<SlotVisual, kMaxVisibleSlots> visuals_;
};
