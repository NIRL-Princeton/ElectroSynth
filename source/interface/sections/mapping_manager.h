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

class ModulationMatrix;
class ModulationMeter;
class ConnectionDestination;
class SynthBase;

struct RegisteredMappingEndpoint {
    juce::Component::SafePointer<juce::Component> component;
    electrosynth::EndpointDescriptor endpoint;
    juce::Component::SafePointer<ConnectionSlots> slots;
};

class MappingManager : public SynthSection, public ConnectionSlots::Listener, public ConnectionButton::Listener,
                        public SynthSlider::SliderListener, public ModulesInterface<ProcessorBase>::Listener,
                        public ModulesInterface<ModulatorBase>::Listener, public AudioChainSection::Listener {

    public:
    static constexpr int kIndicesPerMeter = 6;
    static constexpr float kDragImageWidthPercent = 0.018f;

    MappingManager();
    ~MappingManager();


    enum ConnectionMenuOption {
        kRemoveConnection = 1,
        kToggleConnectionBypass = 2,
        kToggleConnectionBipolar = 3,
        kToggleConnectionStereo = 4
    };

    void registerEndpoint(EndpointArrowComponent& arrow);
    void unregisterEndpoint(const EndpointArrowComponent& endpoint);

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    void createMappingMeter(SynthSlider* slider, OpenGlMultiQuad* quads, int index);
    void createMappingSlider(std::string name, SynthSlider* slider);


    void reset() override;

    void resized() override;
    void updateMappingMeterLocations();
    void connectionAmountChanged(SynthSlider* slider) override;
    void connectionRemoved(SynthSlider* slider) override;

    void connectionSelected(ConnectionButton* source) override;
    void connectionClicked(ConnectionButton* source) override;
    bool hasFreeConnection();
    void startDestinationMap(ConnectionButton* source, const juce::MouseEvent& e) override;
    void mappingDragged(const juce::MouseEvent& e) override;
    void positionDragIcon();
    void connectionWheelMoved(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void clearTemporaryConnection();
    void draggedToComponent(juce::Component* component, bool bipolar);
    void setTemporaryConnectionBipolar(juce::Component* component, bool bipolar);
    void endDestinationMap() override;
    void mappingLostFocus(ConnectionButton* source) override;

    void clearConnectionSource();

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void drawDestinations(OpenGlWrapper& open_gl) const;
    void drawCurrentSource(OpenGlWrapper& open_gl);
    void drawDraggingSource(OpenGlWrapper& open_gl);

    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void renderMeters(OpenGlWrapper& open_gl, bool animate);
    void destroyOpenGlComponents(juce::OpenGLContext& open_gl) override;

    void setVisibleMeterBounds();

    void hoverStarted(SynthSlider* slider) override;
    void hoverEnded(SynthSlider* slider) override;
    void menuFinished(SynthSlider* slider) override;
    void modulationsChanged(const std::string& name) override;
    void mouseDown(SynthSlider* slider) override;
    void mouseUp(SynthSlider* slider) override;
    void doubleClick(SynthSlider* slider) override;
    void sliderValueChanged(juce::Slider* slider) override;


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

    // connection_slot callbacks
    void connectionSlotClicked(const ConnectionSlotData& connection, const juce::MouseEvent& event) override;
    void connectionAmountChanged(const ConnectionSlotData& connection, float amount) override;
    void removeConnectionRecord(const juce::String& connectionId);
    void handleConnectionMenuResult(const juce::String& connectionId, int result);


    void setDestinationQuadBounds(ConnectionDestination* destination);
    bool isPointInsideDestinationDropArea(SynthSlider* slider, juce::Point<int> manager_position) const;
    int findSlotForNewConnection(SynthSlider* slider) const;
    bool isSlotOccupied(const std::string& destination, int destination_slot) const;
    void showConnectionAmountOverlay(const juce::String& connectionId);
    void hideConnectionAmountOverlay();
    void componentAdded();
    void scheduleComponentUpdate();
    void scheduleConnectionSlotRefresh();

    static juce::String getEndpointKey(const electrosynth::EndpointAddress& address);
    void unregisterEndpoint(const electrosynth::EndpointAddress& address);

    void registerEndpoint(juce::Component& component, electrosynth::EndpointDescriptor endpoint, ConnectionSlots* slots);
    RegisteredMappingEndpoint* getRegisteredMappingEndpoint(juce::Component* component);
    static bool endpointsAreCompatible(const electrosynth::EndpointAddress& source, const electrosynth::EndpointAddress& destination);
    RegisteredMappingEndpoint* findEndpointAt(juce::Point<int> managerPosition);
    bool connectEndpoints(const electrosynth::EndpointAddress& source, const electrosynth::EndpointAddress& destination);
    RegisteredMappingEndpoint* getRegisteredMappingEndpoint(const electrosynth::EndpointAddress& address);

    void refreshConnectionSlots();
    void attachAuxiliarySlotData(ConnectionSlotData& slotData, const electrosynth::ConnectionRecord& connection, const SynthGuiInterface& synthInterface);

    std::optional<ConnectionSlotData> makeConnectionSlotData(const electrosynth::ConnectionRecord& connection,
            const electrosynth::EndpointAddress& viewedEndpoint, const electrosynth::EndpointCapabilities& capabilities);

    CriticalSection open_gl_critical_section_;
    std::unique_ptr<juce::Component> destinations_;
    std::map<juce::Viewport*, int> num_rotary_meters;
    std::map<juce::Viewport*, int> num_linear_meters;
    ConnectionButton* current_source_;
    ConnectionDestination* temporarily_set_destination_;
    SynthSlider* temporarily_set_synth_slider_;
    juce::String temporarily_set_connection_id_;
    int temporarily_set_slot_;
    bool temporarily_set_bipolar_;
    OpenGlQuad drag_quad_;
    PlainShapeComponent drag_icon_;

    OpenGlQuad current_quad_;
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
    bool changing_hover_;
    bool component_update_pending_;
    bool connection_slot_refresh_pending_ = false;

    ConnectionButton* current_modulator_;
    std::map<std::string, ConnectionButton*> modulation_buttons_;
    std::map<std::string, ConnectionDestination*> destination_lookup_;
    std::map<std::string, SynthSlider*> slider_model_lookup_;
    std::vector<std::unique_ptr<ConnectionDestination>> all_destinations_;
    std::map<std::string, std::unique_ptr<ModulationMeter>> meter_lookup_;

    void drawMappingMode(OpenGlWrapper& open_gl);
    bool isMappingMode() const;
    OpenGlQuad mapping_mode_dim_quad_;

    std::map<juce::String, RegisteredMappingEndpoint> mapping_endpoints_;
    std::optional<electrosynth::EndpointAddress> endpoint_drag_source_;
    juce::Component::SafePointer<EndpointArrowComponent> endpoint_drag_source_component_;
    std::optional<electrosynth::EndpointAddress> endpoint_drag_destination_;
    juce::Component::SafePointer<EndpointArrowComponent> endpoint_drag_destination_component_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MappingManager)
};
