#include "PluginEditor.h"
#include "interface/FullInterface.h"
#include "interface/look_and_feel/default_look_and_feel.h"
//namespace ProjectInfo
//{
//    const char* const  projectName    = "electrosynth";
//    const char* const  companyName    = "Many Arrows Music";
//    const char* const  versionString  = "3.4a";
//    const int          versionNumber  = 0x30400;
//}
PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), SynthGuiInterface(&p), processorRef (p), was_animating_(true)
{
    juce::ignoreUnused (processorRef);

    setLookAndFeel(DefaultLookAndFeel::instance());
    gui_->reset();
    addAndMakeVisible(gui_.get());

    constrainer_.setMinimumSize(electrosynth::kMinWindowWidth, electrosynth::kMinWindowHeight);
    double ratio = (1.0 * electrosynth::kDefaultWindowWidth) / electrosynth::kDefaultWindowHeight;
    constrainer_.setFixedAspectRatio(ratio);
    constrainer_.setGui(gui_.get());
    setConstrainer(&constrainer_);


    Rectangle<int> total_bounds = Desktop::getInstance().getDisplays().getTotalBounds(true);
    total_bounds.removeFromBottom(50);

    float window_size = 1.0f;
    window_size = std::min(window_size, total_bounds.getWidth() / (1.0f * electrosynth::kDefaultWindowWidth));
    window_size = std::min(window_size, total_bounds.getHeight() / (1.0f * electrosynth::kDefaultWindowHeight));
    int width = std::round(window_size * electrosynth::kDefaultWindowWidth);
    int height = std::round(window_size * electrosynth::kDefaultWindowHeight);

    setResizable(true, true);
    setSize(width, height);
}

PluginEditor::~PluginEditor()
{
}

//void PluginEditor::paint (juce::Graphics& g)
//{
//    // (Our component is opaque, so we must completely fill the background with a solid colour)
//    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
//
//    auto area = getLocalBounds();
//    g.setColour (juce::Colours::white);
//    g.setFont (16.0f);
//    auto helloWorld = juce::String ("Hello from ") + PRODUCT_NAME_WITHOUT_VERSION + " v" VERSION + " running in " + CMAKE_BUILD_TYPE;
//    g.drawText (helloWorld, area.removeFromTop (150), juce::Justification::centred, false);
//}

void PluginEditor::resized() {
    // layout the positions of your child components here
    auto area = getLocalBounds();
    area.removeFromBottom(50);
    AudioProcessorEditor::resized();
    gui_->setBounds(getLocalBounds());
}

void PluginEditor::setScaleFactor(float newScale) {
    AudioProcessorEditor::setScaleFactor(newScale);
    gui_->redoBackground();
}

void PluginEditor::updateFullGui() {
    SynthGuiInterface::updateFullGui();
    //synth_.updateHostDisplay();
}
