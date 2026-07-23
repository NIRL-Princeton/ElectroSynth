#pragma once
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include "synth_section.h"
#include "synth_slider.h"
#include "open_gl_combobox.h"
#include "modulation_slots.h"

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
    juce::Colour getLabelColor(const juce::Component* control) const;

    std::vector<std::unique_ptr<juce::Component>> comps;
    std::unique_ptr<SynthSlider> mix_knob_;
    std::unique_ptr<SynthSlider> postgain_knob_;
    // Filter modules only: placeholder type selector between the module title and the
    // first control row. Unwired — the DSP's FiltType parameter path is stubbed (see
    // tFiltModule_setParameter's empty FiltType case); mirrors the lane header's
    // non-interactive routing dropdown until selection is implemented.
    std::unique_ptr<OpenGLComboBox> filter_type_combo_;
    // White outline for the type dropdown: its fill matches the module body, and the
    // FX bake never calls this view's paintBackground, so the outline is a live quad.
    std::shared_ptr<OpenGlQuad> filter_type_combo_border_;

    std::map<juce::Component*, std::shared_ptr<PlainTextComponent>> slider_labels_;
    std::map<SynthSlider*, std::unique_ptr<ModulationSlots>> modulation_slot_strips_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxModuleTemplateView)
};

} // namespace electrosynth
