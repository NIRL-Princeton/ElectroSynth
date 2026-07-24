//
// Created by Callista Chong on 7/22/26.
//

#pragma once
#include "audio_port_component.h"
#include "synth_section.h"
#include <vector>

class AudioRoutingManager final : public SynthSection, private AudioPortComponent::Listener {
public:
    AudioRoutingManager();

    void registerPort(AudioPortComponent& port);
    void unregisterPort(AudioPortComponent& port);

    void paintBackground(juce::Graphics&) override {}

    void startDestinationMap(AudioPortComponent* source, const juce::MouseEvent& event);
    bool isMappingMode() const;
    void endAudioMap();

    void audioDragged(const juce::MouseEvent& event);
    void audioDraggedToComponent(AudioPortComponent* destination);
    bool isPointInsideDestinationDropArea(AudioPortComponent* destination, juce::Point<int> manager_position) const;
    void positionDragIcon();

private:
    void audioPortDragStarted(AudioPortComponent* port, const juce::MouseEvent& event) override;
    void audioPortDragged(AudioPortComponent* port, const juce::MouseEvent& event) override;
    void audioPortDragEnded(AudioPortComponent* port, const juce::MouseEvent& event) override;

    void drawDraggingAudio(OpenGlWrapper& open_gl);

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
    juce::CriticalSection open_gl_critical_section_;
};