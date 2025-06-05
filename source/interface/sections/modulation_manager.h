/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once



#include "bar_renderer.h"
#include "modulation_button.h"
#include "open_gl_component.h"
#include "open_gl_multi_quad.h"

#include "synth_section.h"
#include "synth_slider.h"
#include <tracktion_ValueTreeUtilities.h>
#include <set>
#include "ModulationModuleSection.h"
#include "SoundModuleSection.h"
class ExpandModulationButton;
class ModulationMatrix;
class ModulationMeter;
class ModulationDestination;
class SynthBase;
namespace electrosynth{
    class ModulationConnectionBank;
}
class ModulationAmountKnob : public SynthSlider {
  public:
    enum MenuOptions {
      kDisconnect = 0xff,
      kToggleBypass,
      kToggleBipolar,
      kToggleStereo,
    };

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void disconnectModulation(ModulationAmountKnob* modulation_knob) = 0;
        virtual void setModulationBypass(ModulationAmountKnob* modulation_knob, bool bypass) = 0;
        virtual void setModulationBipolar(ModulationAmountKnob* modulation_knob, bool bipolar) = 0;
        virtual void setModulationStereo(ModulationAmountKnob* modulation_knob, bool stereo) = 0;
    };

    ModulationAmountKnob(juce::String name, int index, const ValueTree &v);

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void handleModulationMenuCallback(int result);

    void makeVisible(bool visible);
    void hideImmediately();

    void setCurrentModulator(bool current);
    void setDestinationComponent(juce::Component* component, const std::string& name);
    juce::Colour getInternalColor();
    void setSource(const std::string& name);

    juce::Colour withBypassSaturation(juce::Colour color) const {
      if (bypass_)
        return color.withSaturation(0.0f);
      return color;
    }

    virtual juce::Colour getUnselectedColor() const override {
      return withBypassSaturation(SynthSlider::getUnselectedColor());
    }

    virtual juce::Colour getSelectedColor() const override {
      return withBypassSaturation(SynthSlider::getSelectedColor());
    }

    virtual juce::Colour getThumbColor() const override {
      return withBypassSaturation(SynthSlider::getThumbColor());
    }
    void valueChanged() override {
        //DBG("valuechanged");
        SynthSlider::valueChanged();
    }

    void setBypass(bool bypass) { bypass_ = bypass; setColors(); }
    void setStereo(bool stereo) { stereo_ = stereo; }
    void setBipolar(bool bipolar) { bipolar_ = bipolar; }
    bool isBypass() { return bypass_; }
    bool isStereo() { return stereo_; }
    bool isBipolar() { return bipolar_; }
    bool enteringValue() { return text_entry_ && text_entry_->isVisible(); }
    bool isCurrentModulator() { return current_modulator_; }
    int index() { return index_; }

    void setAux(juce::String name) {
      aux_name_ = name;
      setName(aux_name_);
      setModulationAmount(1.0f);
    }
    bool hasAux() { return !aux_name_.isEmpty(); }
    void removeAux() {
      aux_name_ = "";
      setName(name_);
      setModulationAmount(0.0f);
    }
    juce::String getOriginalName() { return name_; }

    force_inline bool hovering() const {
      return hovering_;
    }

    void addModulationAmountListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    void toggleBypass();

    std::vector<Listener*> listeners_;

    juce::Point<int> mouse_down_position_;
    juce::Component* color_component_;
    juce::String aux_name_;
    juce::String name_;
    bool editing_;
    int index_;
    bool showing_;
    bool hovering_;
    bool current_modulator_;
    bool bypass_;
    bool stereo_;
    bool bipolar_;
    bool draw_background_;

    ValueTree state;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationAmountKnob)
};

class ModulationExpansionBox : public OpenGlQuad {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
      
        virtual void expansionFocusLost() = 0;
    };

    ModulationExpansionBox() : OpenGlQuad(Shaders::kRoundedRectangleFragment) { }

    void focusLost(FocusChangeType cause) override {
      OpenGlQuad::focusLost(cause);

      for (Listener* listener : listeners_)
        listener->expansionFocusLost();
    }

    void setAmountControls(std::vector<ModulationAmountKnob*> amount_controls) { amount_controls_ = amount_controls; }
    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    std::vector<ModulationAmountKnob*> amount_controls_;
    std::vector<Listener*> listeners_;
};

class ModulationManager : public SynthSection,
                          public ModulationButton::Listener,
                          public ModulationAmountKnob::Listener,
                          public SynthSlider::SliderListener,
                          public ModulationExpansionBox::Listener,
                          public ModulesInterface<ProcessorBase>::Listener,
                          public ModulesInterface<ModulatorBase>::Listener

{
  public:
    static constexpr int kIndicesPerMeter = 6;
    static constexpr float kDragImageWidthPercent = 0.018f;

    ModulationManager(ValueTree& tree,SynthBase* bank
                      );
    ~ModulationManager();

    void createModulationMeter(
                               SynthSlider* slider, OpenGlMultiQuad* quads, int index);
    void createModulationSlider(std::string name, SynthSlider* slider, bool poly);

    void connectModulation(std::string source, std::string destination);
    void removeModulation(std::string source, std::string destination);
    void setModulationSliderValue(int index, float value);
    void setModulationSliderBipolar(int index, bool bipolar);
    void setModulationSliderValues(int index, float value);
    void setModulationSliderScale(int index);
    void setModulationValues(std::string source, std::string destination,
                             float amount, bool bipolar, bool stereo, bool bypass);
    void reset() override;
    void initAuxConnections();

    void resized() override;
    void parentHierarchyChanged() override;
    void updateModulationMeterLocations();
    void modulationAmountChanged(SynthSlider* slider) override;
    void modulationRemoved(SynthSlider* slider) override;

    void modulationDisconnected(electrosynth::ModulationConnection* connection, bool last) override;
    void modulationSelected(ModulationButton* source) override;
    void modulationClicked(ModulationButton* source) override;
    void modulationCleared() override;
    bool hasFreeConnection();
    void startModulationMap(ModulationButton* source, const juce::MouseEvent& e) override;
    void modulationDragged(const juce::MouseEvent& e) override;
    void modulationWheelMoved(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void clearTemporaryModulation();
    void clearTemporaryHoverModulation();
    void modulationDraggedToHoverSlider(ModulationAmountKnob* hover_slider);
    void modulationDraggedToComponent(juce::Component* component, bool bipolar);
    void setTemporaryModulationBipolar(juce::Component* component, bool bipolar);
    void endModulationMap() override;
    void modulationLostFocus(ModulationButton* source) override;

    void expansionFocusLost() override {
      hideModulationAmountCallout();
    }

    void clearModulationSource();

    void disconnectModulation(ModulationAmountKnob* modulation_knob) override;
    void setModulationSettings(ModulationAmountKnob* modulation_knob);
    void setModulationBypass(ModulationAmountKnob* modulation_knob, bool bypass) override;
    void setModulationBipolar(ModulationAmountKnob* modulation_knob, bool bipolar) override;
    void setModulationStereo(ModulationAmountKnob* modulation_knob, bool stereo) override;

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void drawModulationDestinations(OpenGlWrapper& open_gl);
    void drawCurrentModulator(OpenGlWrapper& open_gl);
    void drawDraggingModulation(OpenGlWrapper& open_gl);
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void renderMeters(OpenGlWrapper& open_gl, bool animate);
    void renderSourceMeters(OpenGlWrapper& open_gl, int index);
    void updateSmoothModValues();
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;
    void paintBackground(juce::Graphics& g) override { positionModulationAmountSliders(); }

    void setModulationAmounts();
    void setVisibleMeterBounds();

    void hoverStarted(SynthSlider* slider) override;
    void hoverEnded(SynthSlider* slider) override;
    void menuFinished(SynthSlider* slider) override;
    void modulationsChanged(const std::string& name) override;
    int getIndexForModulationSlider(juce::Slider* slider);
    int getModulationIndex(std::string source, std::string destination);
    electrosynth::ModulationConnection* getConnectionForModulationSlider(juce::Slider* slider);
    electrosynth::ModulationConnection* getConnection(int index);
    electrosynth::ModulationConnection* getConnection(const std::string& source, const std::string& dest);
    void mouseDown(SynthSlider* slider) override;
    void mouseUp(SynthSlider* slider) override;
    void doubleClick(SynthSlider* slider) override;
    void beginModulationEdit(SynthSlider* slider) override;
    void endModulationEdit(SynthSlider* slider) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void hideUnusedHoverModulations();
    float getAuxMultiplier(int index);
    void addAuxConnection(int from_index, int to_index);
    void removeAuxDestinationConnection(int to_index);
    void removeAuxSourceConnection(int from_index);


    // this will be called by both
    //  ModulesInterface<ModulationSection>::Listener,
    //    ModulesInterface<ModuleSection>::Listener
    void added() override
    {
        componentAdded();
    }
    void removed() override
    {
        componentAdded();
    }
    void effectsMoved() override
    {
        return;
    }
  private:

    void setDestinationQuadBounds(ModulationDestination* destination);
    void makeCurrentModulatorAmountsVisible();
    void makeModulationsVisible(SynthSlider* destination, bool visible);
    void positionModulationAmountSlidersInside(const std::string& source,
                                               std::vector<electrosynth::ModulationConnection*> connections);
    void positionModulationAmountSlidersCallout(const std::string& source,
                                                std::vector<electrosynth::ModulationConnection*> connections);
    void showModulationAmountCallout(const std::string& source);
    void hideModulationAmountCallout();
    void positionModulationAmountSliders(const std::string& source);
    void positionModulationAmountSliders();
    bool enteringHoverValue();
    void showModulationAmountOverlay(ModulationAmountKnob* slider);
    void hideModulationAmountOverlay();
    void componentAdded();

    CriticalSection open_gl_critical_section_;
    juce::ValueTree state_;
    std::unique_ptr<juce::Component> modulation_destinations_;
    std::map<juce::Viewport*, int> num_rotary_meters;
    std::map<juce::Viewport*, int> num_linear_meters;
    ModulationButton* current_source_;
    ExpandModulationButton* current_expanded_modulation_;
    ModulationDestination* temporarily_set_destination_;
    SynthSlider* temporarily_set_synth_slider_;
    ModulationAmountKnob* temporarily_set_hover_slider_;
    bool temporarily_set_bipolar_;
    OpenGlQuad drag_quad_;
    std::shared_ptr<ModulationExpansionBox> modulation_expansion_box_;
    OpenGlQuad current_modulator_quad_;
    OpenGlQuad editing_rotary_amount_quad_;
    OpenGlQuad editing_linear_amount_quad_;
    std::map<juce::Viewport*, std::shared_ptr<OpenGlMultiQuad>> rotary_destinations_;
    std::map<juce::Viewport*, std::unique_ptr<OpenGlMultiQuad>> linear_destinations_;
    std::map<juce::Viewport*, std::shared_ptr<OpenGlMultiQuad>> rotary_meters_;
    std::map<juce::Viewport*, std::unique_ptr<OpenGlMultiQuad>> linear_meters_;
    juce::Point<int> mouse_drag_start_;
    juce::Point<int> mouse_drag_position_;
    bool modifying_;
    bool dragging_;
    bool changing_hover_modulation_;

    ModulationButton* current_modulator_;
    std::map<std::string, ModulationButton*> modulation_buttons_;
    std::map<std::string, std::unique_ptr<ExpandModulationButton>> modulation_callout_buttons_;

    std::map<std::string, bool> active_mod_values_;
    long long last_milliseconds_;
    std::unique_ptr<BarRenderer> modulation_source_meters_;

    std::map<std::string, ModulationDestination*> destination_lookup_;
    std::map<std::string, SynthSlider*> slider_model_lookup_;
    std::map<std::string, ModulationAmountKnob*> modulation_amount_lookup_;

    std::vector<std::unique_ptr<ModulationDestination>> all_destinations_;
    std::map<std::string, std::unique_ptr<ModulationMeter>> meter_lookup_;
    std::map<int, int> aux_connections_from_to_;
    std::map<int, int> aux_connections_to_from_;
    std::unique_ptr<ModulationAmountKnob> modulation_amount_sliders_[electrosynth::kMaxModulationConnections];
    std::unique_ptr<ModulationAmountKnob> modulation_hover_sliders_[electrosynth::kMaxModulationConnections];
    std::unique_ptr<ModulationAmountKnob> selected_modulation_sliders_[electrosynth::kMaxModulationConnections];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationManager)
};

