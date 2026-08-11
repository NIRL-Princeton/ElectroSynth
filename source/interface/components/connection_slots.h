//
// Created by Callista Chong on 7/25/26.
//

#pragma once
#include <array>
#include <optional>
#include <vector>
#include "ConnectionRecord.h"
#include "open_gl_multi_quad.h"
#include "synth_section.h"
#include "synth_slider.h"

class EndpointArrowComponent;

struct ConnectionSlotData {
    juce::String connectionId;
    electrosynth::EndpointAddress peer;
    juce::String label;
    juce::Colour colour;

    bool hasAmount = false;
    bool hasBipolar = false;
    bool hasStereo = false;
    float amount = 1.0f;            // set default modulation amount

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
        using ClickCallback = std::function<void(int, const juce::MouseEvent&)>;
        using AmountCallback = std::function<void(int, float)>;

        using HoverCallback = std::function<void(int, bool)>;

        SlotComponent(juce::String componentId, int slotIndex, std::function<void()> onChange,
                      ClickCallback onClick, AmountCallback onAmountChanged, HoverCallback hoverCallback );

        void paint(juce::Graphics& g) override;
        int getSlotIndex() const { return slot_index_; }

        void setConnection(ConnectionSlotData connection);
        void clearConnection();

        const ConnectionSlotData* getConnection() const noexcept;

        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;

        void mouseEnter(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;


    private:
        void notifySlotHost();
        std::optional<ConnectionSlotData> connection_;
        int slot_index_;
        std::function<void()> on_change_;
        ClickCallback on_click_;

        AmountCallback on_amount_changed_;
        float drag_start_amount_ = 1.0f;
        int drag_start_y_ = 0;

        bool draw_background_;

        juce::String source_name_;
        bool editing_;
        int index_;
        bool showing_;
        bool hovering_;
        bool current_source_;

        HoverCallback on_hover_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlotComponent)
    };
}


class ConnectionSlots final : public SynthSection {

public:
    class Listener {
        public:
        virtual ~Listener() = default;

        virtual void connectionSlotClicked(const ConnectionSlotData& connection, const juce::MouseEvent& event) = 0;
        virtual void connectionAmountChanged(const ConnectionSlotData& connection, float amount) = 0;
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    static constexpr int kMaxVisibleSlots = 4;
    static constexpr int kSlotWidth = 44;
    static constexpr int kSlotHeight = 14;
    static constexpr int kSlotGap = 2;
    static constexpr int kSlotPitch = kSlotWidth + kSlotGap;
    static constexpr int kPreferredWidth =
        kMaxVisibleSlots * kSlotWidth + (kMaxVisibleSlots - 1) * kSlotGap;

    explicit ConnectionSlots(EndpointArrowComponent& endpoint_arrow);
    explicit ConnectionSlots(SynthSlider& destination);
    ~ConnectionSlots() override;

    void setConnections(std::vector<ConnectionSlotData> connections);
    void resized() override;
    void paintBackground(Graphics&) override {}

    void slotHoverChanged(int index, bool hovering);
    void showSlotPopup(int index);

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

    int hovered_slot_ = -1;
    std::vector<Listener*> listeners_;

    void initialiseSlot(int index, const juce::String& prefix);
    void slotClicked(int index, const juce::MouseEvent& event);
    void slotAmountChanged(int index, float amount);
    void syncOpenGl();
    EndpointArrowComponent* arrow_ = nullptr;
    SynthSlider* destination_ = nullptr;
    std::array<std::unique_ptr<electrosynth::SlotComponent>, kMaxVisibleSlots> slot_components_;
    std::array<SlotVisual, kMaxVisibleSlots> visuals_;
};
