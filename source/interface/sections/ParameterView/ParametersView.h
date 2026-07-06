#pragma once

#include <chowdsp_plugin_state/chowdsp_plugin_state.h>
#include "open_gl_image_component.h"
#include "open_gl_multi_quad.h"
#include "synth_section.h"
#include "synth_slider.h"
struct OpenGlWrapper;

namespace electrosynth {
    class ModulationSlotComponent : public juce::Component {
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
	        void clearSource() {
	            setSourceName({});
	            setSourceDisplayLabel({});
	            setModulationAmount(0.0f);
	            setBypass(false);
	            setAuxSource({}, {});
	        }
	        bool isOccupied() const { return source_name_.isNotEmpty(); }
	        juce::Colour getSourceColor() const;
	        juce::String getSourceLabel() const;
	        float getModulationAmount() const { return modulation_amount_; }
	        bool isBypass() const { return bypass_; }
	        bool hasAuxSource() const { return aux_source_name_.isNotEmpty(); }
	        juce::Colour getAuxSourceColor() const;
	        juce::String getAuxSourceLabel() const;

    private:
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
        void syncModulationSlotOpenGl();
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
        void updateSliderLabels();
//        struct Pimpl;
//        std::unique_ptr<Pimpl> pimpl;
        std::vector<std::unique_ptr<juce::Component>> comps;
        std::map<juce::Component*, std::shared_ptr<PlainTextComponent>> slider_labels_;
        std::map<juce::Component*, juce::Rectangle<int>> modulation_boxes_;
        using ModulationSlots =
            std::array<std::unique_ptr<ModulationSlotComponent>, SynthSlider::kNumModulationSlots>;
        std::map<juce::Component*, ModulationSlots> modulation_box_targets_;
	        struct ModulationSlotOpenGl {
	            std::shared_ptr<OpenGlQuad> body;
	            std::shared_ptr<OpenGlQuad> amount;
	            std::shared_ptr<OpenGlQuad> border;
	            std::shared_ptr<PlainTextComponent> label;
	            std::shared_ptr<OpenGlQuad> aux_body;
	            std::shared_ptr<OpenGlQuad> aux_border;
	            std::shared_ptr<PlainTextComponent> aux_label;
	        };
        using ModulationSlotOpenGlSet =
            std::array<ModulationSlotOpenGl, SynthSlider::kNumModulationSlots>;
        std::map<juce::Component*, ModulationSlotOpenGlSet> modulation_box_open_gl_;
        bool vertically_center_knobs_ = false;
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
