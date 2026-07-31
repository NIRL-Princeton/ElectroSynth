//
// Created by Davis Polito on 11/8/23.
//

#ifndef BITKLAVIER2_FULLINTERFACE_H
#define BITKLAVIER2_FULLINTERFACE_H


//#include "open_gl_background.h"
#include "header_section.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include "look_and_feel/skin.h"
#include "main_section.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "popup_browser.h"
#include "synth_section.h"
#include "value_tree_debugger.h"

#include <functional>
#include <utility>
class AboutSection;
struct SynthGuiData;
class HeaderSection;
class MainSection;
class ModulationManager;
namespace electrosynth{
    constexpr int kMinWindowWidth = 350;
    constexpr int kMinWindowHeight = 205;
    constexpr int kDefaultWindowWidth = 1400;
    constexpr int kDefaultWindowHeight = 820;
}
class FullInterface : public SynthSection, public juce::OpenGLRenderer, public HeaderSection::Listener,
                      public MainSection::Listener, juce::DragAndDropContainer, private juce::Timer
{

public :
    static constexpr double kMinOpenGlVersion = 1.4;
    FullInterface(SynthGuiData *synth_gui_data);
     ~FullInterface() override;

     void paintBackground(juce::Graphics& g) override;

    void parentHierarchyChanged() override {
        SynthSection::parentHierarchyChanged();
        checkShouldReposition();
    }
    void timerCallback()   // to avoid flickering when resizing the window
    {

//        open_gl_context_.attachTo(*this);
//        stopTimer();
    }
    void copySkinValues(const Skin& skin);
    void reloadSkin(const Skin& skin);
    void applySkinToSubtree(SynthSection* section);
    void showAboutSection() override;
    void sendToDeviceRequested() override;
    void setSelectedMidiOutputProvider(std::function<juce::MidiOutput*()> provider);
    bool sendMidiBufferToSelectedOutput(const juce::MidiBuffer& midi_messages);
    void repaintChildBackground(SynthSection* child);
    void repaintSynthesisSection();
    void repaintOpenGlBackground(OpenGlComponent* component);
    void redoBackground();
    void checkShouldReposition(bool resize = true);
    void resized() override;
    void animate(bool animate) override;
    void reset() override;

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void setFocus();
    void notifyChange();
    void notifyFresh();
    float getResizingScale() const { return width_ * 1.0f / resized_width_; }
    //float getPixelScaling() const override { return display_scale_; }
    int getPixelMultiple() const override { return pixel_multiple_; }
   // std::unique_ptr<HeaderSection> header_;
    void showFullScreenSection(SynthSection* full_screen);
    void enableRedoBackground(bool enable) {
        enable_redo_background_ = enable;
        if (enable)
            resized();
    }
    void hideDisplay(bool primary);
    void popupSelector(juce::Component* source, juce::Point<int> position, const PopupItems& options,
        std::function<void(int)> callback, std::function<void()> cancel);

    void popupDisplay(juce::Component* source, const std::string& text,
        juce::BubbleComponent::BubblePlacement placement, bool primary);

    //void prepDisplay(PreparationSection* source);
    std::unique_ptr<SinglePopupSelector> popup_selector_;
    //std::unique_ptr<PreparationPopup> prep_popup;
    std::unique_ptr<PopupDisplay> popup_display_1_;
    std::unique_ptr<PopupDisplay> popup_display_2_;
    SynthGuiData* data;
    SynthSection* full_screen_section_;
    juce::OpenGLContext open_gl_context_;
    juce::CriticalSection open_gl_critical_section_;
    OpenGlWrapper open_gl_;
    std::map<std::string, SynthSlider*> getAllSliders() override;
    std::map<std::string, ModulationButton*> getAllModulationButtons() override;
    void modulationChanged();
    juce::ScopedPointer<ValueTreeDebugger> valueTreeDebugger;
private :
    std::unique_ptr<AboutSection> about_section_;
    std::unique_ptr<MainSection> main_;
    std::unique_ptr<HeaderSection> header_;
//std::unique_ptr<TestSection> test_;
    int width_;
    int resized_width_;
    bool animate_;
    bool unsupported_{};
    float last_render_scale_;
    bool enable_redo_background_{};
    float display_scale_;
    int pixel_multiple_;

    //juce::OpenGLContext open_gl_context_;
    std::unique_ptr<Shaders> shaders_;
    Skin current_skin_;

    juce::Image background_image_;
    //juce::TextButton inspectButton { "Inspect the UI" };
    // std::unique_ptr<melatonin::Inspector> inspector;
    //std::unique_ptr<OpenGlToggleButton> inspectButton;
    OpenGlBackground background_;
    std::unique_ptr<ModulationManager> modulation_manager;
    std::function<juce::MidiOutput*()> selected_midi_output_provider_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FullInterface)
};

#endif //BITKLAVIER2_FULLINTERFACE_H
