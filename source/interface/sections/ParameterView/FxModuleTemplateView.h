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

private:
    static constexpr int kMaxEffectSlots = 5;

    std::vector<std::unique_ptr<juce::Component>> comps;
    std::vector<std::unique_ptr<SynthSlider>> placeholders_;
    std::unique_ptr<SynthSlider> mix_knob_;
    std::unique_ptr<SynthSlider> postgain_knob_;

    std::array<juce::Rectangle<int>, 7> mod_boxes_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxModuleTemplateView)
};

} // namespace electrosynth
