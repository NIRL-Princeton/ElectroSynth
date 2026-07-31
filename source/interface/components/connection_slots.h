//
// Created by Callista Chong on 7/25/26.
//

#pragma once
#include "audio_port_component.h"
#include "synth_slider.h"
#include "open_gl_multi_quad.h"
#include "synth_section.h"
#include <array>
#include <vector>
#include <optional>

struct ConnectionSlotData {
    juce::String connectionId;
    electrosynth::EndpointAddress peer;
    juce::String label;
    juce::Colour colour;

    bool hasAmount = false;
    bool hasBipolar = false;
    float amount = 1.0f;

    bool bipolar = false;
    bool bypass = false;
    bool stereo = false;

    struct Auxiliary {
        juce::String connectionId;
        electrosynth::EndpointAddress peer;
        juce::String label;
        juce::Colour colour;
    };
    std::optional<Auxiliary> auxiliary;
};

namespace electrosynth {

    class SlotComponent final : public juce::Component {
    public:
        SlotComponent(juce::String componentId, int slot_index, std::function<void()> onChange);

        void paint(juce::Graphics& g) override;
        int getSlotIndex() const { return slot_index_; }

        void setConnection(ConnectionSlotData connection);
        void clearConnection();

        bool hasConnection() const noexcept;
        const ConnectionSlotData* getConnection() const noexcept;

    private:
        void notifySlotHost();
        std::optional<ConnectionSlotData> connection_;
        std::function<void()> on_change_;
        int slot_index_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlotComponent)
    };
}


class ConnectionSlots final : public SynthSection {

public:
    static constexpr int kMaxVisibleSlots = 4;
    static constexpr int kSlotWidth = 44;
    static constexpr int kSlotHeight = 14;
    static constexpr int kSlotGap = 2;
    static constexpr int kSlotPitch = kSlotWidth + kSlotGap;
    static constexpr int kPreferredWidth =
        kMaxVisibleSlots * kSlotWidth + (kMaxVisibleSlots - 1) * kSlotGap;

    explicit ConnectionSlots(AudioPortComponent& port);
    explicit ConnectionSlots(SynthSlider& destination);
    ~ConnectionSlots() override;

    void setConnections(std::vector<ConnectionSlotData> connections);
    void resized() override;
    void paintBackground(Graphics&) override {}


private:
    struct SlotVisual {
        std::shared_ptr<OpenGlQuad> body;
        std::shared_ptr<OpenGlQuad> amount;
        std::shared_ptr<OpenGlQuad> border;
        std::shared_ptr<PlainTextComponent> label;
        std::shared_ptr<OpenGlQuad> aux_body;
        std::shared_ptr<OpenGlQuad> aux_border;
        std::shared_ptr<PlainTextComponent> aux_label;
    };

    void initialiseSlot(int index, const juce::String& prefix);
    void syncOpenGl();
    AudioPortComponent* port_ = nullptr;
    SynthSlider* destination_ = nullptr;
    std::array<std::unique_ptr<electrosynth::SlotComponent>, kMaxVisibleSlots> slot_components_;
    std::array<SlotVisual, kMaxVisibleSlots> visuals_;
};
