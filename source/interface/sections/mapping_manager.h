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
#include "connection_button.h"
#include "open_gl_component.h"
#include "open_gl_multi_quad.h"

#include "synth_section.h"
#include "synth_slider.h"
#include <tracktion_ValueTreeUtilities.h>
#include <set>

#include "AudioChainSection.h"
#include "ModulationModuleSection.h"
#include "SoundModuleSection.h"

#include "ConnectionRecord.h"
#include "endpoint_arrow_component.h"

class ExpandConnectionButton;
class ModulationMatrix;
class ModulationMeter;
class ConnectionDestination;
class SynthBase;

namespace electrosynth{
    struct Connection;
    class ConnectionBank;
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
        virtual void disconnectConnection(ModulationAmountKnob* modulation_knob) = 0;
        virtual void setConnectionBypass(ModulationAmountKnob* modulation_knob, bool bypass) = 0;
        virtual void setConnectionBipolar(ModulationAmountKnob* modulation_knob, bool bipolar) = 0;
        virtual void setConnectionStereo(ModulationAmountKnob* modulation_knob, bool stereo) = 0;
    };

    ModulationAmountKnob(juce::String name, int index, const ValueTree &v);


    void handleModulationMenuCallback(int result);

    void makeVisible(bool visible);
    void hideImmediately();

    void setCurrentSource(bool current);

    void setSource(const std::string& name);
    juce::String getSourceName() const noexcept { return source_name_; }
    juce::String getSourceLabel() const;
    juce::Colour getSourceColor() const;

    juce::Colour withBypassSaturation(juce::Colour color) const {
      if (bypass_)
        return color.withSaturation(0.0f);
      return color;
    }

    juce::Colour getUnselectedColor() const override {
      return withBypassSaturation(SynthSlider::getUnselectedColor());
    }

    juce::Colour getSelectedColor() const override {
      return withBypassSaturation(SynthSlider::getSelectedColor());
    }

    juce::Colour getThumbColor() const override {
      return withBypassSaturation(SynthSlider::getThumbColor());
    }


    void setBypass(bool bypass) {
        bypass_ = bypass;
        setColors();
        redoImage();
        repaint();
    }
    void setStereo(bool stereo) { stereo_ = stereo; }
    void setBipolar(bool bipolar) { bipolar_ = bipolar; }
    bool isBypass() { return bypass_; }
    bool isStereo() { return stereo_; }
    bool isBipolar() { return bipolar_; }
    bool enteringValue() { return text_entry_ && text_entry_->isVisible(); }
    bool isCurrentModulator() { return current_source_; }
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
    juce::String source_name_;
    bool editing_;
    int index_;
    bool showing_;
    bool hovering_;
    bool current_source_;
    bool bypass_;
    bool stereo_;
    bool bipolar_;
    bool draw_background_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationAmountKnob)
};

class SlotExpansionBox : public OpenGlQuad {
  public:

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void expansionFocusLost() = 0;
    };

    SlotExpansionBox() : OpenGlQuad(Shaders::kRoundedRectangleFragment) { }

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

struct RegisteredMappingEndpoint {
    juce::Component::SafePointer<EndpointArrowComponent> component;
};

class MappingManager : public SynthSection, public ConnectionSlots::Listener, public ConnectionButton::Listener, public ModulationAmountKnob::Listener,
                        public SynthSlider::SliderListener, public SlotExpansionBox::Listener, public ModulesInterface<ProcessorBase>::Listener,
                        public ModulesInterface<ModulatorBase>::Listener, public AudioChainSection::Listener {

    public:
    static constexpr int kIndicesPerMeter = 6;
    static constexpr float kDragImageWidthPercent = 0.018f;

    MappingManager(ValueTree& tree,SynthBase* bank);
    ~MappingManager();


    enum ConnectionMenuOption {
        kRemoveConnection = 1,
        kToggleConnectionBypass = 2,
        kToggleConnectionBipolar = 3,
        kToggleConnectionStereo = 4
    };

    void registerEndpoint(EndpointArrowComponent& endpoint);
    void unregisterEndpoint(const EndpointArrowComponent& endpoint);

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    void createMappingMeter(SynthSlider* slider, OpenGlMultiQuad* quads, int index);
    void createMappingSlider(std::string name, SynthSlider* slider);


    bool connectMapping(std::string source, std::string destination, int destination_slot = -1);
    void removeMapping(std::string source, std::string destination, int destination_slot = -1);
    void setConnectionSliderValue(int index, float value);
    void setConnectionSliderBipolar(int index, bool bipolar);
    void setConnectionSliderValues(int index, float value);
    void setConnectionSliderScale(int index);
    void setConnectionValues(std::string source, std::string destination,
                             float amount, bool bipolar, bool stereo, bool bypass,
                             int destination_slot = -1);
    void reset() override;
    void initAuxConnections();

    void resized() override;
    void updateMappingMeterLocations();
    void connectionAmountChanged(SynthSlider* slider) override;
    void connectionRemoved(SynthSlider* slider) override;

    void connectionSelected(ConnectionButton* source) override;
    void connectionClicked(ConnectionButton* source) override;
    bool hasFreeConnection();
    void startDestinationMap(ConnectionButton* source, const juce::MouseEvent& e) override;
    void startEndpointMap(EndpointArrowComponent* source, const juce::MouseEvent& e);
    void mappingDragged(const juce::MouseEvent& e) override;
    void positionDragIcon();
    void connectionWheelMoved(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void clearTemporaryConnection();
    void clearTemporaryHoverConnection();
    void makeAuxilaryConnection(ModulationAmountKnob* hover_slider);
    void draggedToComponent(juce::Component* component, bool bipolar);
    void setTemporaryConnectionBipolar(juce::Component* component, bool bipolar);
    void endDestinationMap() override;
    void mappingLostFocus(ConnectionButton* source) override;

    void expansionFocusLost() override {
      hideConnectionAmountCallout();
    }

    void clearConnectionSource();

    void disconnectConnection(ModulationAmountKnob* modulation_knob) override;
    void setConnectionSettings(ModulationAmountKnob* modulation_knob);
    void setConnectionBypass(ModulationAmountKnob* modulation_knob, bool bypass) override;
    void setConnectionBipolar(ModulationAmountKnob* modulation_knob, bool bipolar) override;
    void setConnectionStereo(ModulationAmountKnob* modulation_knob, bool stereo) override;

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void drawDestinations(OpenGlWrapper& open_gl);
    void drawCurrentSource(OpenGlWrapper& open_gl);
    void drawDraggingSource(OpenGlWrapper& open_gl);

    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void renderMeters(OpenGlWrapper& open_gl, bool animate);
    void destroyOpenGlComponents(juce::OpenGLContext& open_gl) override;
    void paintBackground(juce::Graphics& g) override { positionConnectionAmountSliders(); }

    void setConnectionAmounts();
    void setVisibleMeterBounds();

    void hoverStarted(SynthSlider* slider) override;
    void hoverEnded(SynthSlider* slider) override;
    void menuFinished(SynthSlider* slider) override;
    void modulationsChanged(const std::string& name) override;
    int getIndexForModulationSlider(juce::Slider* slider);
    int getModulationIndex(std::string source, std::string destination, int destination_slot = -1);
    electrosynth::ConnectionRecord* getConnectionForSlider(juce::Slider* slider);
    electrosynth::ConnectionRecord* getConnection(int index);
    electrosynth::ConnectionRecord* getConnection(const std::string& source, const std::string& dest,
                                                      int destination_slot = -1);
    void mouseDown(SynthSlider* slider) override;
    void mouseUp(SynthSlider* slider) override;
    void doubleClick(SynthSlider* slider) override;
    void beginConnectionEdit(SynthSlider* slider);
    void endConnectionEdit(SynthSlider* slider);
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
    void added() override { componentAdded(); }
    void removed() override {
        // Defer + coalesce the modulation rebuild instead of running it inline. Calling
        // componentAdded() synchronously from inside EffectModuleSection::removeModule's
        // listener->removed() path re-enters the GL dispatch mid-removal and crashes.
        scheduleComponentUpdate();
    }
    void effectsMoved() override { return; }


private:

    void updateEndpointDestinationVisuals();
    void clearEndpointDestinationVisuals();
    void positionEndpointDragIcon();
    void drawEndpointDestinations(OpenGlWrapper& openGl);
    juce::String getCurrentSourceName() const;

    electrosynth::ConnectionRecord* findConnectionRecord(const juce::String& connectionId);
    const electrosynth::ConnectionRecord* findConnectionRecord(const juce::String& connectionId) const;
    electrosynth::ConnectionRecord* findConnectionRecord(const std::string& source, const std::string& destination,
                                                        int destination_slot = -1,
                                                        electrosynth::ConnectionType type = electrosynth::ConnectionType::Modulation);

    // connection_slot callbacks
    void connectionSlotClicked(const ConnectionSlotData& connection, const juce::MouseEvent& event) override;
    void connectionAmountChanged(const ConnectionSlotData& connection, float amount) override;
    void removeConnectionRecord(const juce::String& connectionId);
    void handleConnectionMenuResult(const juce::String& connectionId, int result);


    void setDestinationQuadBounds(ConnectionDestination* destination);
    bool isPointInsideDestinationDropArea(SynthSlider* slider, juce::Point<int> manager_position) const;
    int findSlotForNewConnection(SynthSlider* slider) const;
    bool isSlotOccupied(const std::string& destination, int destination_slot) const;
    void updateSlotVisuals();
    void makeCurrentConnectionAmountsVisible();
    void makeConnectionsVisible(SynthSlider* destination, bool visible);
    ModulationAmountKnob* getConnectionAmountControl(const electrosynth::ConnectionRecord* connection) const;
    void syncConnectionAmountControl(const electrosynth::ConnectionRecord* connection, ModulationAmountKnob* amount_knob);
    bool placeConnectionAmountInSlot(SynthSlider* destination, const electrosynth::ConnectionRecord* connection,
                                     ModulationAmountKnob* amount_knob);
    void showConnectionAmountCallout(const std::string& source);
    void hideConnectionAmountCallout();
    void positionConnectionAmountSliders();
    bool enteringHoverValue();
    void showConnectionAmountOverlay(const juce::String& connectionId);
    void hideConnectionAmountOverlay();
    void componentAdded();
    void scheduleComponentUpdate();

    static juce::String getEndpointKey(const electrosynth::EndpointAddress& address);
    void unregisterEndpoint(const electrosynth::EndpointAddress& address);
    RegisteredMappingEndpoint* getRegisteredMappingEndpoint(juce::Component* component);
    RegisteredMappingEndpoint* getRegisteredMappingEndpointRecursive(juce::Component* component);
    bool endpointsAreCompatible(const electrosynth::EndpointAddress& source, const electrosynth::EndpointAddress& destination) const;
    RegisteredMappingEndpoint* findEndpointAt(juce::Point<int> managerPosition);
    bool connectEndpoints(const electrosynth::EndpointAddress& source, const electrosynth::EndpointAddress& destination);
    RegisteredMappingEndpoint* getRegisteredMappingEndpoint(const electrosynth::EndpointAddress& address);
    void updateConnectionSlots();

    CriticalSection open_gl_critical_section_;
    juce::ValueTree state_;
    std::unique_ptr<juce::Component> destinations_;
    std::map<juce::Viewport*, int> num_rotary_meters;
    std::map<juce::Viewport*, int> num_linear_meters;
    EndpointArrowComponent* current_source_;
    ExpandConnectionButton* current_expanded_;
    ConnectionDestination* temporarily_set_destination_;
    SynthSlider* temporarily_set_synth_slider_;
    ModulationAmountKnob* temporarily_set_hover_slider_;
    int temporarily_set_slot_;
    bool temporarily_set_bipolar_;
    OpenGlQuad drag_quad_;
    PlainShapeComponent drag_icon_;
    std::shared_ptr<SlotExpansionBox> expansion_box_;

    OpenGlQuad current_quad_;
    OpenGlQuad editing_rotary_amount_quad_;
    OpenGlQuad editing_linear_amount_quad_;
    std::map<juce::Viewport*, std::shared_ptr<OpenGlMultiQuad>> rotary_destinations_;
    std::map<juce::Viewport*, std::unique_ptr<OpenGlMultiQuad>> linear_destinations_;
    std::map<juce::Viewport*, std::shared_ptr<OpenGlMultiQuad>> rotary_meters_;
    std::map<juce::Viewport*, std::unique_ptr<OpenGlMultiQuad>> linear_meters_;

    struct StaticModulationArc {
        electrosynth::ConnectionRecord* connection = nullptr;
        ModulationMeter* meter = nullptr;
        std::unique_ptr<OpenGlQuad> quad;
        int ring_index = 0;
        juce::Colour color = juce::Colours::white;
    };

    std::vector<StaticModulationArc> static_modulation_arcs_;

    juce::Point<int> mouse_drag_start_;
    juce::Point<int> mouse_drag_position_;
    bool modifying_;
    bool dragging_;
    bool changing_hover_;
    bool component_update_pending_;

    ConnectionButton* current_modulator_;
    std::map<std::string, ConnectionButton*> modulation_buttons_;
    std::map<std::string, std::unique_ptr<ExpandConnectionButton>> callout_buttons_;

    std::map<std::string, bool> active_mod_values_;

    std::map<std::string, ConnectionDestination*> destination_lookup_;
    std::map<std::string, SynthSlider*> slider_model_lookup_;
    std::map<std::string, ModulationAmountKnob*> modulation_amount_lookup_;

    std::vector<std::unique_ptr<ConnectionDestination>> all_destinations_;
    std::map<std::string, std::unique_ptr<ModulationMeter>> meter_lookup_;
    std::map<int, int> aux_connections_from_to_;
    std::map<int, int> aux_connections_to_from_;
    std::unique_ptr<ModulationAmountKnob> modulation_icon_[electrosynth::kMaxConnections];

    void drawMappingMode(OpenGlWrapper& open_gl);
    bool isMappingMode() const;
    OpenGlQuad mapping_mode_dim_quad_;

    std::map<juce::String, RegisteredMappingEndpoint> mapping_endpoints_;
    std::optional<electrosynth::EndpointAddress> endpoint_drag_source_;
    juce::Component::SafePointer<EndpointArrowComponent> endpoint_drag_source_component_;
    std::optional<electrosynth::EndpointAddress> endpoint_drag_destination_;
    juce::Component::SafePointer<EndpointArrowComponent> endpoint_drag_destination_component_;

    std::vector<electrosynth::ConnectionRecord> connection_records_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MappingManager)
};
