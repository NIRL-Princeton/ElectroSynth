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

struct ConnectionSlotData {
    juce::String connectionId;
    electrosynth::EndpointAddress peer;
    juce::String label;
    juce::Colour colour;

    bool hasAmount = false;
    bool hasBipolar = false;
    float amount = 1.0f;
};

namespace electrosynth {

    class SlotComponent final : public juce::Component {
    public:
        SlotComponent(juce::String componentId, int slot_index, std::function<void()> onChange);

        void paint(juce::Graphics& g) override;
        int getSlotIndex() const { return slot_index_; }

        void setSourceName(juce::String source_name);
        void setSourceDisplayLabel(juce::String display_label);
        void setModulationAmount(float amount);
        void setBypass(bool bypass);
        void setAuxSource(juce::String source_name, juce::String display_label);
        void clearSource();

        bool isOccupied() const { return source_name_.isNotEmpty(); }
        juce::Colour getSourceColor() const;
        juce::String getSourceLabel() const;
        float getModulationAmount() const { return modulation_amount_; }
        bool isBypass() const { return bypass_; }
        bool hasAuxSource() const { return aux_source_name_.isNotEmpty(); }
        juce::Colour getAuxSourceColor() const;
        juce::String getAuxSourceLabel() const;



    private:
        void notifySlotHost();
        juce::Colour getColorForSource(const juce::String& source_name) const;
        juce::String getLabelForSource(const juce::String& source_name,
                                   const juce::String& display_label) const;


        std::function<void()> on_change_;
        int slot_index_;
        juce::String source_name_;
        juce::String display_label_;
        juce::String aux_source_name_;
        juce::String aux_display_label_;
        float modulation_amount_ = 0.0f;
        bool bypass_ = false;

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
    int getConnectionCount() const noexcept {
        return static_cast<int>(connections_.size());
    }
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

    void syncOpenGl();
    std::vector<ConnectionSlotData> connections_;
    AudioPortComponent* port_ = nullptr;
    SynthSlider* destination_ = nullptr;
    std::array<std::unique_ptr<electrosynth::SlotComponent>, kMaxVisibleSlots> slot_components_;
    std::array<SlotVisual, kMaxVisibleSlots> visuals_;
};
