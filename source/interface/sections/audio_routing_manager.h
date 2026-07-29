//
// Created by Callista Chong on 7/22/26.
//

#pragma once
#include "audio_port_component.h"
#include "AudioConnection.h"
#include "synth_section.h"
#include <vector>
#include <optional>

class AudioRoutingManager final : public SynthSection, private AudioPortComponent::Listener {
public:
    AudioRoutingManager();

    class Listener {
    public:
        virtual ~Listener() = default;

        virtual void audioConnectionCreated(const electrosynth::audio::AudioConnection& connection) = 0;
        virtual void audioConnectionRemoved(const electrosynth::audio::AudioConnection& connection) = 0;
    };

    void addListener(Listener* listener) { listeners_.add(listener); }
    void removeListener(Listener* listener) { listeners_.remove(listener); }

    struct AudioPortDisplayInfo {
        juce::String name;
        juce::Colour colour;
    };
    
    void registerPort(AudioPortComponent& port);
    void unregisterPort(AudioPortComponent& port);

    void paintBackground(juce::Graphics&) override {}

    void startDestinationMap(AudioPortComponent* source, const juce::MouseEvent& event);
    void drawDestinationHighlights(OpenGlWrapper& open_gl);
    bool isMappingMode() const;
    void endAudioMap();

    void audioDragged(const juce::MouseEvent& event);
    void audioDraggedToComponent(AudioPortComponent* destination);
    bool isValidDestination(const AudioPortComponent* port) const;
    bool isPointInsideDestinationDropArea(AudioPortComponent* destination, juce::Point<int> manager_position) const;
    void positionDragIcon();

private:
    juce::ListenerList<Listener> listeners_;

    void audioPortDragStarted(AudioPortComponent* port, const juce::MouseEvent& event) override;
    void audioPortDragged(AudioPortComponent* port, const juce::MouseEvent& event) override;
    void audioPortDragEnded(AudioPortComponent* port, const juce::MouseEvent& event) override;

    void drawDraggingAudio(OpenGlWrapper& open_gl);
    void drawMappingMode(OpenGlWrapper& open_gl);

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(juce::OpenGLContext& open_gl) override;

    using SafePort =
        juce::Component::SafePointer<AudioPortComponent>;
    std::vector<SafePort> ports_;

    AudioPortComponent* current_source_ = nullptr;
    SafePort current_destination_;

    juce::Point<int> mouse_drag_position_;
    bool dragging_ = false;

    PlainShapeComponent drag_icon_;
    OpenGlQuad mapping_mode_dim_quad_;
    static constexpr int kMaxVisiblePorts = 128;
    juce::CriticalSection open_gl_critical_section_;

    bool connectAudio(const electrosynth::audio::AudioConnection& connection);
    std::optional<electrosynth::audio::AudioConnection> getConnectionTo(const electrosynth::audio::AudioPortAddress& destination) const;
    void updatePortConnectionSlots();

    std::vector<electrosynth::audio::AudioConnection> connections_;
};
