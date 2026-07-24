#pragma once

#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include "open_gl_image_component.h"
#include "open_gl_multi_quad.h"
#include "modulation_slots.h"
#include "synth_section.h"
#include "synth_slider.h"
struct OpenGlWrapper;

namespace electrosynth {
/** Clone of juce::GenericAudioProcessorEditor, but usable as a generic component */
    class ParametersView : public SynthSection {
    public:
        ParametersView (chowdsp::PluginState& pluginState, chowdsp::ParamHolder& params,String name);
        ParametersView (chowdsp::ParameterListeners& paramListeners, chowdsp::ParamHolder& params,String name);
        ~ParametersView() override;

        void paint(juce::Graphics &) override;
        void paintBackground(juce::Graphics& g) override;

        void resized() override;
        int getPreferredHeight() const override;
        void setVerticallyCenterKnobs(bool should_center) {
            vertically_center_knobs_ = should_center;
            resized();
        }
//        void initOpenGlComponents(OpenGlWrapper &open_gl) override;
//        void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
        void init_();
        /** Returns nullptr if no component is found for the given parameter */
        [[nodiscard]] juce::Component* getComponentForParameter (const juce::RangedAudioParameter&);
        void mouseEnter (const MouseEvent& event)
        {
            DBG("mouseenter parameterview");
        }
    private:
        static constexpr int kDefaultKnobsPerRow = 7;
        static constexpr int kStringKnobsPerRow = 6;
        static constexpr int kModuleHeightPerKnobRow = 110;
        static constexpr int kKnobLabelHeight = 18;
        static constexpr int kKnobLabelGap = 4;

        static constexpr int kModulationBoxHeight = 16;
        static constexpr int kModulationBoxGap = 4;
        static constexpr int kModulationBoxWidth = 120;

        int getKnobsPerRow() const;
        int getKnobRowCount() const;
        juce::Colour getSliderLabelColor() const;
        void ensureSliderLabels();
        void ensureModulationSlots(SynthSlider& slider);
        void updateSliderLabels();
//        struct Pimpl;
//        std::unique_ptr<Pimpl> pimpl;
        std::vector<std::unique_ptr<juce::Component>> comps;
        std::map<juce::Component*, std::shared_ptr<PlainTextComponent>> slider_labels_;
        std::map<SynthSlider*, std::unique_ptr<ModulationSlots>> modulation_slot_strips_;
        bool vertically_center_knobs_ = false;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametersView)
    };
    /** Clone of juce::GenericAudioProcessorEditor. */
    class ParametersViewEditor : public juce::AudioProcessorEditor { // redundant
    public:
        template <typename PluginType>
        explicit ParametersViewEditor (PluginType& plugin, String name)
            : ParametersViewEditor (plugin, plugin.getState(), plugin.getState().params,name ) {
        }

        ParametersViewEditor (juce::AudioProcessor& proc, chowdsp::PluginState& pluginState, chowdsp::ParamHolder& params,String name)
            : juce::AudioProcessorEditor (proc), view (pluginState, params,name) {
            //setResizable (true, false);
            //setSize (view.getWidth(), view.getHeight());
            //addAndMakeVisible (view);
        }

        void resized() override {
            //view.setBounds (getLocalBounds());
        }

        ParametersView view;

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametersViewEditor)
    };
///** Clone of juce::GenericAudioProcessorEditor. */
//    class ParametersViewEditor : public juce::AudioProcessorEditor {
//    public:
//        ParametersViewEditor(juce::AudioProcessor &proc, chowdsp::PluginState &pluginState,
//                             chowdsp::ParamHolder &params)
//                : juce::AudioProcessorEditor(proc),
//                  view(pluginState, params, ) {
//            setResizable(true, false);
//            setSize(view.getWidth(), view.getHeight());
//
//            addAndMakeVisible(view);
//        }
//
//        void resized() override {
//            view.setBounds(getLocalBounds());
//        }
//
//    private:
//        ParametersView view;
//
//        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametersViewEditor)
//    };
}//namespace bitilavier
