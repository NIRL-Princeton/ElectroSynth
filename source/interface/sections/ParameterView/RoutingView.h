//
// Created by Davis Polito on 7/11/25.
//

#ifndef ROUTINGVIEW_H
#define ROUTINGVIEW_H
#include "synth_section.h"
#include "default_look_and_feel.h"
#include "open_gl_combobox.h"
#include "RoutingProcessor.h"
#include "synth_slider.h"

class RoutingGainSlider : public SynthSlider
{
public:
    using SynthSlider::SynthSlider;

    juce::Colour getSelectedColor() const override {
        return findColour(Skin::kWidgetPrimary1, true);
    }

    juce::Colour getUnselectedColor() const override {
        return findColour(Skin::kWidgetBackground, true);
    }

    juce::Colour getThumbColor() const override {
        return findColour(Skin::kWidgetPrimary1, true);
    }

    juce::Colour getBackgroundColor() const override {
        return findColour(Skin::kBackground, true);
    }
};


class RoutingView : public SynthSection {
public:
    RoutingView(chowdsp::PluginState& pluginState, RoutingParams& params,String name): SynthSection("RoutingView") {
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);
        setInterceptsMouseClicks(false, true);
        auto& listeners = pluginState.getParameterListeners();

        // master gain slider for each osc/string component
        gain_slider = std::make_unique<RoutingGainSlider> (params.gainparam->paramID);
        gain_slider_attachment = std::make_unique<chowdsp::SliderAttachment> (*params.gainparam.get(), listeners, *gain_slider.get(), nullptr);
        gain_slider->setSliderStyle (juce::Slider::LinearBar);
        gain_slider->setComponentID(params.gainparam->paramID);
        setLookAndFeel(DefaultLookAndFeel::instance());
        gain_slider->setScrollWheelEnabled(false);
        addSlider(gain_slider.get(), true);

        // combo box (gain/ lane 1/ lane 2)
        routing_combo_box  = std::make_unique<OpenGLComboBox>();
        routing_combo_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*params.routing.get(), listeners, *routing_combo_box.get(), nullptr);

        addAndMakeVisible (routing_combo_box.get());
        addOpenGlComponent(routing_combo_box->getImageComponent());
    }
    void paintBackground(juce::Graphics& g) override {
        paintBorder(g);
        paintKnobShadows(g);
        paintChildrenBackgrounds(g);
    }

    void resized() override {
        auto bounds = getLocalBounds().reduced(30, 8);
        auto slider_area = bounds.removeFromLeft(static_cast<int>(bounds.getWidth() * 0.3f));
        auto combo_area = bounds.removeFromRight(static_cast<int>(bounds.getWidth() * 0.5f));

        gain_slider->setBounds(slider_area);
        gain_slider->redoImage();
        routing_combo_box->setBounds(combo_area);
    }
    std::unique_ptr<SynthSlider> gain_slider;
    std::unique_ptr<chowdsp::SliderAttachment> gain_slider_attachment;
    std::unique_ptr<OpenGLComboBox> routing_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> routing_combo_attachment;
};
#endif //ROUTINGVIEW_H
