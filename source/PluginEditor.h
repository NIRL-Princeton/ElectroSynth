#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "synth_gui_interface.h"
#include "border_bounds_constrainer.h"


//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor, public SynthGuiInterface
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override { }
    void resized() override;
    void setScaleFactor(float newScale) override;

    void updateFullGui() override;
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PluginProcessor& processorRef;
    BorderBoundsConstrainer constrainer_;

    bool was_animating_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
