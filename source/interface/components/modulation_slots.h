//
// Created by Callista Chong on 7/22/26.
//

#pragma once
#include "synth_section.h"
#include "synth_slider.h"

namespace electrosynth {

    class ModulationSlots;
    class ModulationSlotComponent final : public juce::Component {
        public:
        ModulationSlotComponent(SynthSlider& destination_slider, int slot_index);

        void paint(juce::Graphics& g) override;

        SynthSlider& getDestinationSlider() const { return destination_slider_; }
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

        SynthSlider& destination_slider_;
        int slot_index_;
        juce::String source_name_;
        juce::String display_label_;
        juce::String aux_source_name_;
        juce::String aux_display_label_;
        float modulation_amount_ = 0.0f;
        bool bypass_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationSlotComponent)
};

class ModulationSlots final : public SynthSection {
public:
    static constexpr int kHeight = 16;

    explicit ModulationSlots(SynthSlider& destination);
    ~ModulationSlots() override;

    void resized() override;
    void paintBackground(juce::Graphics&) override { }
    void syncOpenGl();

private:
    struct SlotVisuals {
        std::shared_ptr<OpenGlQuad> body;
        std::shared_ptr<OpenGlQuad> amount;
        std::shared_ptr<OpenGlQuad> border;
        std::shared_ptr<PlainTextComponent> label;
        std::shared_ptr<OpenGlQuad> aux_body;
        std::shared_ptr<OpenGlQuad> aux_border;
        std::shared_ptr<PlainTextComponent> aux_label;
    };

    SynthSlider& destination_;
    std::array<std::unique_ptr<ModulationSlotComponent>, SynthSlider::kNumModulationSlots> slots_;
    std::array<SlotVisuals, SynthSlider::kNumModulationSlots> visuals_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationSlots)
};

} // namespace electrosynth
