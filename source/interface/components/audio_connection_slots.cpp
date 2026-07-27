//
// Created by Callista Chong on 7/25/26.
//

#include "audio_connection_slots.h"

AudioConnectionSlots::AudioConnectionSlots(AudioPortComponent& port)
    : SynthSection(port.getComponentID() + "_connection_slots"), port_(port) {

    setAlwaysOnTop(true);
    setInterceptsMouseClicks(false, false);

    for (int index = 0; index < kMaxVisibleSlots; ++index) {
        auto& visual = visuals_[index];
        const auto prefix = getName() + "_" + juce::String(index);


        visual.body = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleFragment, prefix + "_body");
        visual.body->setInterceptsMouseClicks(false, false);
        addOpenGlComponent(visual.body);

        visual.border = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleBorderFragment, prefix + "_border");
        visual.border->setInterceptsMouseClicks(false, false);
        visual.border->setThickness(1.0f, true);
        addOpenGlComponent(visual.border);

        visual.label = std::make_shared<PlainTextComponent>(prefix + "_label", "");
        visual.label->setInterceptsMouseClicks(false, false);
        visual.label->setFontType(PlainTextComponent::kRegular);
        visual.label->setJustification(juce::Justification::centred);
        addOpenGlComponent(visual.label);
    }
    port_.setConnectionSlots(this);
}

AudioConnectionSlots::~AudioConnectionSlots() {
    if (port_.getConnectionSlots() == this)
        port_.setConnectionSlots(nullptr);
}

void AudioConnectionSlots::setConnections(std::vector<AudioConnectionSlot> connections) {
    connections_ = std::move(connections);
    const int visible_count = juce::jmin(static_cast<int>(connections_.size()), kMaxVisibleSlots);

    for (int index = 0; index < kMaxVisibleSlots; ++index) {
        const bool visible = index < visible_count;
        visuals_[index].body->setVisible(visible);
        visuals_[index].border->setVisible(visible);
        visuals_[index].label->setVisible(visible);
    }

    setVisible(visible_count > 0);
    if (visible_count > 0)
        syncOpenGl();
}

void AudioConnectionSlots::resized() {
    SynthSection::resized();

    const bool is_input = port_.getAddress().direction == electrosynth::audio::PortDirection::Input;
    const int slot_y = (getHeight() - kSlotHeight) / 2;

    for (int index = 0; index < kMaxVisibleSlots; ++index) {
        const int slot_x = is_input ? index * kSlotPitch : getWidth() - kSlotWidth - index * kSlotPitch;
        const juce::Rectangle<int> bounds {slot_x, slot_y, kSlotWidth, kSlotHeight};

        visuals_[index].body->setBounds(bounds);
        visuals_[index].border->setBounds(bounds);
        visuals_[index].label->setBounds(bounds.reduced(2, 1));
    }
    syncOpenGl();
}

void AudioConnectionSlots::syncOpenGl() {
    const auto rounding = juce::jmin(findValue(Skin::kWidgetRoundedCorner),
        static_cast<float>(kSlotHeight) * 0.5f);

    for (int index = 0; index < kMaxVisibleSlots; ++index) {
        auto& visual = visuals_[index];
        const bool occupied = index < static_cast<int>(connections_.size());
        const auto colour = occupied ? connections_[index].colour : juce::Colours::transparentBlack;

        visual.body->setColor(colour.withAlpha(0.28f));
        visual.body->setRounding(rounding);
        visual.border->setColor(colour);
        visual.border->setRounding(rounding);
        visual.label->setText(occupied ? connections_[index].label : "");
        visual.label->setTextSize(9.0f);
        visual.label->setColor(colour);
    }
}
