#pragma once
#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
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

private:
    static constexpr int kMaxEffectSlots = 5;

    void ensureLabels();
    void updateLabels();
    juce::Colour getLabelColor(const juce::Component* control) const;

    std::vector<std::unique_ptr<juce::Component>> comps;
    std::unique_ptr<SynthSlider> mix_knob_;
    std::unique_ptr<SynthSlider> postgain_knob_;

    std::map<juce::Component*, std::shared_ptr<PlainTextComponent>> slider_labels_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxModuleTemplateView)
};

} // namespace electrosynth
