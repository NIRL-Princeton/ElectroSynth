#pragma once
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include "open_gl_combobox.h"
#include "connection_slots.h"
#include "synth_section.h"
#include "synth_slider.h"

namespace electrosynth {

class FxModuleTemplateView : public SynthSection {
public:
    FxModuleTemplateView(chowdsp::PluginState& pluginState,
                         chowdsp::ParamHolder& params,
                         juce::String name);
    ~FxModuleTemplateView() override;

    void resized() override;
    void paintBackground(juce::Graphics& g) override;

    // FX-only: height derived from the dynamic row layout so the module shrinks/grows
    // with its visible-control count instead of filling the full viewport height.
    int getPreferredHeight() const override;

    // Right edge of the PostGain knob (tick-arc box) in this view's coordinates, or -1
    // before layout. Used by the owning ModuleSection to align its delete button.
    int getPostGainRightEdge() const {
        if (postgain_knob_ == nullptr || postgain_knob_->getBounds().isEmpty())
            return -1;
        return postgain_knob_->getBounds().getRight();
    }

private:
    static constexpr int kMaxEffectSlots = 5;

    void ensureLabels();
    void ensureModulationSlots(SynthSlider& slider);
    void updateLabels();
    juce::String getControlLabel(const juce::Component& control) const;
    juce::Colour getLabelColor(const juce::Component* control) const;

    std::vector<std::unique_ptr<juce::Component>> comps;
    std::unique_ptr<SynthSlider> mix_knob_;
    std::unique_ptr<SynthSlider> postgain_knob_;
    // Filter modules only: the attached type selector between the module title and
    // the first control row.
    std::unique_ptr<OpenGLComboBox> filter_type_combo_;
    std::unique_ptr<chowdsp::ComboBoxAttachment> filter_type_attachment_;
    int filter_type_index_ = 0;

    std::map<juce::Component*, std::shared_ptr<PlainTextComponent>> slider_labels_;
    std::map<SynthSlider*, std::unique_ptr<ConnectionSlots>> modulation_slot_strips_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxModuleTemplateView)
};

} // namespace electrosynth
