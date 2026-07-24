//
// Created by Callista Chong on 7/21/26.
//

#pragma once

#include "AudioNode.h"
#include "open_gl_image_component.h"
#include "paths.h"

// this is the visible, clickable arrow corresponding to one AudioPortAddress (see AudioNode.h)

class AudioPortComponent : public PlainShapeComponent {

public:
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void audioPortDragStarted(AudioPortComponent*, const juce::MouseEvent&) {}
        virtual void audioPortDragged(AudioPortComponent*, const juce::MouseEvent&) {}
        virtual void audioPortDragEnded(AudioPortComponent*, const juce::MouseEvent&) {}
    };

    AudioPortComponent(juce::String name, electrosynth::audio::AudioPortAddress address);

    const electrosynth::audio::AudioPortAddress& getAddress() const noexcept {
        return address_;
    }

    void addListener(Listener* listener) { listeners_.add(listener); }
    void removeListener(Listener* listener) { listeners_.remove(listener); }

    void resized() override {
        PlainShapeComponent::resized();
        redrawImage(true);
    }

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    electrosynth::audio::AudioPortAddress address_;
    juce::ListenerList<Listener> listeners_;
    void setArrowScale(float scale);
};
