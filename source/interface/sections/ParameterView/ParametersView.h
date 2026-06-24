#pragma once

#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include "open_gl_image_component.h"
#include "synth_section.h"
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
        int getKnobsPerRow() const;
        int getKnobRowCount() const;
        void ensureSliderLabels();
        void updateSliderLabels();
//        struct Pimpl;
//        std::unique_ptr<Pimpl> pimpl;
        std::vector<std::unique_ptr<juce::Component>> comps;
        std::map<juce::Component*, std::shared_ptr<PlainTextComponent>> slider_labels_;
        std::map<juce::Component*, juce::Rectangle<int>> modulation_boxes_;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametersView)
    };
    /** Clone of juce::GenericAudioProcessorEditor. */
    class ParametersViewEditor : public juce::AudioProcessorEditor
    {
    public:
        template <typename PluginType>
        explicit ParametersViewEditor (PluginType& plugin, String name)
            : ParametersViewEditor (plugin, plugin.getState(), plugin.getState().params,name )
        {
        }

        ParametersViewEditor (juce::AudioProcessor& proc, chowdsp::PluginState& pluginState, chowdsp::ParamHolder& params,String name)
            : juce::AudioProcessorEditor (proc),
              view (pluginState, params,name)
        {
//            setResizable (true, false);
//            setSize (view.getWidth(), view.getHeight());

            //addAndMakeVisible (view);
        }

        void resized() override
        {
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
