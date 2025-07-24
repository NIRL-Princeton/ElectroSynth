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
class RoutingView : public SynthSection {
public:
    RoutingView(chowdsp::PluginState& pluginState, RoutingParams& params,String name): SynthSection("RoutingView") {
        setLookAndFeel (DefaultLookAndFeel::instance());
        setComponentID (name);
        auto& listeners = pluginState.getParameterListeners();
        gain_slider = std::make_unique<SynthSlider> (params.gainparam->paramID);
        gain_slider_attachment = std::make_unique<chowdsp::SliderAttachment> (*params.gainparam.get(), listeners, *gain_slider.get(), nullptr);

        gain_slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        gain_slider->setComponentID(params.gainparam->paramID);
        setLookAndFeel(DefaultLookAndFeel::instance());
       gain_slider->setScrollWheelEnabled(false);
        addSlider(gain_slider.get(), true);
        routing_combo_box  = std::make_unique<OpenGLComboBox>();
        routing_combo_attachment = std::make_unique<chowdsp::ComboBoxAttachment>(*params.routing.get(), listeners, *routing_combo_box.get(), nullptr);

        addAndMakeVisible (routing_combo_box.get());
        addOpenGlComponent(routing_combo_box->getImageComponent());
    }
    void paintBackground(juce::Graphics& g) override
    {
        // SynthSection::paintContainer(g);
        // paintHeadingText(g);
        // paintBorder(g);
        paintKnobShadows(g);
        // for (auto& slider : _sliders) {
        //     drawLabelForComponent(g, slider->getName(), slider.get());
        // }
        drawLabelBackgroundForComponent (g,gain_slider.get());
        //(dg,gain_slider.get());
        paintChildrenBackgrounds(g);
    }

    void resized() override {
        auto bounds = getLocalBounds();
        auto fromleft = bounds.removeFromLeft(bounds.getWidth() / 4);
        auto toright = bounds.removeFromRight(bounds.getWidth() /2);
        gain_slider->setBounds(fromleft);
        routing_combo_box->setBounds(toright);
    }
    std::unique_ptr<SynthSlider> gain_slider;
    std::unique_ptr<chowdsp::SliderAttachment> gain_slider_attachment;
    std::unique_ptr<OpenGLComboBox> routing_combo_box;
    std::unique_ptr<chowdsp::ComboBoxAttachment> routing_combo_attachment;
};
#endif //ROUTINGVIEW_H
