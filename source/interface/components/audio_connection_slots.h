//
// Created by Callista Chong on 7/25/26.
//

#pragma once
#include "audio_port_component.h"
#include "open_gl_multi_quad.h"
#include "synth_section.h"
#include <array>
#include <vector>

struct AudioConnectionSlot {
    electrosynth::audio::AudioPortAddress peer;
    juce::String label;
    juce::Colour colour;
};

class AudioConnectionSlots final : public SynthSection {

public:
    static constexpr int kMaxVisibleSlots = 4;
    static constexpr int kSlotWidth = 60;
    static constexpr int kSlotHeight = 16;
    static constexpr int kSlotGap = 2;
    static constexpr int kSlotPitch = kSlotWidth + kSlotGap;
    static constexpr int kPreferredWidth =
        kMaxVisibleSlots * kSlotWidth + (kMaxVisibleSlots - 1) * kSlotGap;

    explicit AudioConnectionSlots(AudioPortComponent& port);
    ~AudioConnectionSlots() override;

    void setConnections(std::vector<AudioConnectionSlot> connections);
    int getConnectionCount() const noexcept {
        return static_cast<int>(connections_.size());
    }
    void resized() override;
    void paintBackground(Graphics&) override {}

private:
    struct SlotVisual {
        std::shared_ptr<OpenGlQuad> body;
        std::shared_ptr<OpenGlQuad> border;
        std::shared_ptr<PlainTextComponent> label;
    };

    void syncOpenGl();
    AudioPortComponent& port_;
    std::vector<AudioConnectionSlot> connections_;
    std::array<SlotVisual, kMaxVisibleSlots> visuals_;
};
