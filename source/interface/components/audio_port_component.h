//
// Created by Callista Chong on 7/21/26.
//

#pragma once

#include "AudioNode.h"
#include "open_gl_image_component.h"
#include "paths.h"

class AudioConnectionSlots;

// this is the visible, clickable arrow corresponding to one AudioPortAddress (see AudioNode.h)
// all this does is know my node/port ID, input/output, and detects mouse events.

class AudioPortComponent : public PlainShapeComponent {

public:
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void audioPortDragStarted(AudioPortComponent*, const juce::MouseEvent&) {}
        virtual void audioPortDragged(AudioPortComponent*, const juce::MouseEvent&) {}
        virtual void audioPortDragEnded(AudioPortComponent*, const juce::MouseEvent&) {}
    };

    void addListener(Listener* listener) { listeners_.add(listener); }
    void removeListener(Listener* listener) { listeners_.remove(listener); }

    AudioPortComponent(juce::String name, electrosynth::audio::AudioPortAddress address);

    const electrosynth::audio::AudioPortAddress& getAddress() const noexcept {
        return address_;
    }

    void resized() override {
        PlainShapeComponent::resized();
        redrawImage(true);
    }

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    void setConnectionSlots(AudioConnectionSlots* slots) noexcept;
    AudioConnectionSlots* getConnectionSlots() const noexcept;

    void setMappingTarget(bool target);
    void setDragTarget(bool target);
    void render(OpenGlWrapper& open_gl, bool animate) override;

private:
    electrosynth::audio::AudioPortAddress address_;
    juce::ListenerList<Listener> listeners_;
    void setArrowScale(float scale);
    AudioConnectionSlots* connection_slots_ = nullptr;

    void updateArrowScale();

    bool mouse_hovered_ = false;
    bool mapping_target_ = false;
    bool drag_target_ = false;
};
