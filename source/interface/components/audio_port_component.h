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
    AudioPortComponent(juce::String name, electrosynth::audio::AudioPortAddress address)
        : PlainShapeComponent(std::move(name)), address_(std::move(address)) {

        using electrosynth::audio::PortDirection;
        setShape(Paths::rightArrow());
        setActive(true);
        setUseAlpha(true);
        setInterceptsMouseClicks(true, false);
    }

    const electrosynth::audio::AudioPortAddress& getAddress() const noexcept {
        return address_;
    }

    void resized() override {
        PlainShapeComponent::resized();
        redrawImage(true);
    }

private:
    electrosynth::audio::AudioPortAddress address_;
};


