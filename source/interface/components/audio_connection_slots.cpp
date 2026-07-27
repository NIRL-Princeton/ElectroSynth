//
// Created by Callista Chong on 7/25/26.
//

#include "audio_connection_slots.h"

namespace {
    juce::Colour getDestinationColour(const electrosynth::audio::AudioPortAddress& destination, juce::Colour base_colour) {
        const auto destination_key = destination.nodeId + ":" + destination.portId;
        const auto hash = static_cast<juce::uint32>(destination_key.hashCode());
        const auto hue_rotation = static_cast<float>(hash % 360u) / 360.0f;

        return base_colour.withRotatedHue(hue_rotation);
    }
}

// AudioConnectionSlots owns multiple instances of AudioConnectionSlotComponent, including their OpenGl visuals
class AudioConnectionSlotComponent : public juce::Component {

public:
    void setPeer(electrosynth::audio::AudioPortAddress peer, juce::String peerName, juce::Colour peerColour);
    void clear();
    const juce::String& getPeerName() const;
    juce::Colour getPeerColour() const;

private:
    electrosynth::audio::AudioPortAddress peer_;
    juce::String peer_name_;
    juce::Colour peer_colour_;
};

AudioConnectionSlots::AudioConnectionSlots(AudioPortComponent& port)
    : SynthSection(port.getComponentID() + "_connection_slots"), port_(port) {

    setAlwaysOnTop(true);
    setInterceptsMouseClicks(false, false);

    for (int index = 0; index < kMaxVisibleSlots; ++index) {
        auto& visual = visuals_[index];
        const auto prefix = getName() + "_" + juce::String(index);

        visual.body = std::make_shared<OpenGlQuad>(
            Shaders::kRoundedRectangleFragment, prefix + "_body");
        visual.border = std::make_shared<OpenGlQuad>(
            Shaders::kRoundedRectangleBorderFragment, prefix + "_border");

        visual.body->setInterceptsMouseClicks(false, false);
        visual.border->setInterceptsMouseClicks(false, false);
        visual.border->setThickness(1.0f, true);

        addOpenGlComponent(visual.body);
        addOpenGlComponent(visual.border);
    }

    port_.setConnectionSlots(this);
}

AudioConnectionSlots::~AudioConnectionSlots() {
    if (port_.getConnectionSlots() == this)
        port_.setConnectionSlots(nullptr);
}

void AudioConnectionSlots::setDestinations(
    std::vector<electrosynth::audio::AudioPortAddress> destinations) {
    destinations_ = std::move(destinations);
    const int visible_count = juce::jmin(
        static_cast<int>(destinations_.size()), kMaxVisibleSlots);

    for (int index = 0; index < kMaxVisibleSlots; ++index) {
        const bool visible = index < visible_count;
        visuals_[index].body->setVisible(visible);
        visuals_[index].border->setVisible(visible);
    }

    setVisible(visible_count > 0);

    if (visible_count > 0)
        syncOpenGl();
}

void AudioConnectionSlots::resized() {
    SynthSection::resized();

    const bool is_input = port_.getAddress().direction == electrosynth::audio::PortDirection::Input;
    const int slot_y = (getHeight() - kSlotSize) / 2;

    for (int index = 0; index < kMaxVisibleSlots; ++index) {
        const int slot_x = is_input
            ? index * kSlotPitch
            : getWidth() - kSlotSize - index * kSlotPitch;
        const juce::Rectangle<int> bounds {
            slot_x, slot_y, kSlotSize, kSlotSize
        };

        visuals_[index].body->setBounds(bounds);
        visuals_[index].border->setBounds(bounds);
    }

    syncOpenGl();
}

void AudioConnectionSlots::syncOpenGl() {
    const auto base_colour =
        getLookAndFeel().findColour(Skin::kWidgetAccent1);
    const auto rounding = juce::jmin(
        findValue(Skin::kWidgetRoundedCorner),
        static_cast<float>(kSlotSize) * 0.5f);

    for (int index = 0; index < kMaxVisibleSlots; ++index) {
        auto& visual = visuals_[index];
        const auto colour = index < static_cast<int>(destinations_.size())
            ? getDestinationColour(destinations_[index], base_colour)
            : base_colour;

        visual.body->setColor(colour.withAlpha(0.28f));
        visual.body->setRounding(rounding);
        visual.border->setColor(colour);
        visual.border->setRounding(rounding);
    }
}
