#pragma once

#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
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

        void resized() override;
//        void initOpenGlComponents(OpenGlWrapper &open_gl) override;
//        void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
        void init_();
        /** Returns nullptr if no component is found for the given parameter */
        [[nodiscard]] juce::Component* getComponentForParameter (const juce::RangedAudioParameter&);
        void paintBackground(juce::Graphics& g) override
        {
            SynthSection::paintContainer(g);
            paintHeadingText(g);
            paintBorder(g);
            paintKnobShadows(g);
            paintChildrenBackgrounds(g);
            for (auto slider : all_sliders_v)
            {
                drawLabelForComponent(g, slider->getName(), slider);
            }
        }

        void mouseEnter (const MouseEvent& event)
        {
            DBG("mouseenter parameterview");
        }
    private:
//        struct Pimpl;
//        std::unique_ptr<Pimpl> pimpl;
        std::vector<std::unique_ptr<juce::Component>> comps;
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
