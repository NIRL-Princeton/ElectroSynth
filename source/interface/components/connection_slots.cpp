//
// Created by Callista Chong on 7/25/26.
//

#include "connection_slots.h"

#include "endpoint_arrow_component.h"

namespace {
    juce::Rectangle<int> getAuxSlotBounds(juce::Rectangle<int> slotBounds) {
        auto bounds = slotBounds.reduced(2, 2);
        bounds.setTop(slotBounds.getBottom());
        return bounds;
    }

    juce::Colour getBypassAdjustedColor(juce::Colour color, bool bypass) {
        return bypass
            ? color.withSaturation(0.0f).withMultipliedBrightness(0.85f)
            : color;
    }
}

namespace electrosynth {
    SlotComponent::SlotComponent(juce::String componentId, int slotIndex,
                                 std::function<void()> onChange,
                                 ClickCallback onClick,
                                 AmountCallback onAmountChanged) :
        slot_index_(slotIndex), on_change_(std::move(onChange)),
        on_click_(std::move(onClick)), on_amount_changed_(std::move(onAmountChanged)) {

        setComponentID(std::move(componentId));
        setInterceptsMouseClicks(true, false);
    }

    void SlotComponent::paint(juce::Graphics&) { }

    void SlotComponent::notifySlotHost() {
        repaint();
        if (on_change_) // refresh openGl
            on_change_();
    }

    void SlotComponent::setConnection (ConnectionSlotData connection) {
        connection_ = std::move(connection);
        notifySlotHost();
    }

    void SlotComponent::clearConnection() {
        if (!connection_)
            return;

        connection_.reset();
        notifySlotHost();
    }


    const ConnectionSlotData* SlotComponent::getConnection() const noexcept {
        return connection_ ? &*connection_ : nullptr;
    }


    void SlotComponent::mouseDown(const juce::MouseEvent& event) {
        if (!connection_) return;
        if (on_click_) on_click_(slot_index_, event);

        if (!event.mods.isPopupMenu() && connection_->hasAmount) {
            drag_start_amount_ = connection_->amount;
            drag_start_y_ = event.getScreenY();
        }
    }

    void SlotComponent::mouseDrag(const juce::MouseEvent& event) {

        if (!connection_ || !connection_->hasAmount || event.mods.isPopupMenu())
            return;

        constexpr float dragDistance = 100.0f;
        const float delta = static_cast<float>(drag_start_y_ - event.getScreenY()) / dragDistance;

        const float minimum = connection_->hasBipolar ? -1.0f : 0.0f;
        const float amount = juce::jlimit(minimum, 1.0f, drag_start_amount_ + delta);

        if (amount == connection_->amount)
            return;

        connection_->amount = amount;
        notifySlotHost();

        if (on_amount_changed_)
            on_amount_changed_(slot_index_, amount);
    }
}

void ConnectionSlots::initialiseSlot(int index, const juce::String& prefix) {
    auto& visual = visuals_[index];
    visual.body = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, prefix + "_body");
    visual.amount = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, prefix + "_amount");
    visual.border = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleBorderFragment, prefix + "_border");
    visual.label = std::make_shared<PlainTextComponent>(prefix + "_label", "");
    visual.aux_body = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, prefix + "_aux_body");
    visual.aux_border = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleBorderFragment, prefix + "_aux_border");
    visual.aux_label = std::make_shared<PlainTextComponent>(prefix + "_aux_label", "");

    for (auto* quad : { visual.body.get(), visual.amount.get(), visual.border.get(),
                        visual.aux_body.get(), visual.aux_border.get() }) {
        quad->setInterceptsMouseClicks(false, false);
        quad->setAlwaysOnTop(true);
    }

    for (auto* label : { visual.label.get(), visual.aux_label.get() }) {
        label->setInterceptsMouseClicks(false, false);
        label->setAlwaysOnTop(true);
        label->setFontType(PlainTextComponent::kRegular);
        label->setJustification(juce::Justification::centred);
    }

    visual.border->setThickness(1.0f, true);
    visual.aux_border->setThickness(1.0f, true);

    addOpenGlComponent(visual.body);
    addOpenGlComponent(visual.amount);
    addOpenGlComponent(visual.border);
    addOpenGlComponent(visual.label);
    addOpenGlComponent(visual.aux_body);
    addOpenGlComponent(visual.aux_border);
    addOpenGlComponent(visual.aux_label);

    auto slot = std::make_unique<electrosynth::SlotComponent>(
        prefix + "_component", index, [this] { syncOpenGl(); },
        [this](int slotIndex, const juce::MouseEvent& event) {
            slotClicked(slotIndex, event);
        },
        [this](int slotIndex, float amount) {
            slotAmountChanged(slotIndex, amount);
        });

    auto* slot_component = slot.get();
    addAndMakeVisible(slot_component);

    if (destination_ != nullptr)
        destination_->setExtraModulationTarget(index, slot_component);

    slot_components_[index] = std::move(slot);
}

ConnectionSlots::ConnectionSlots(EndpointArrowComponent& endpoint_arrow)
    : SynthSection(endpoint_arrow.getComponentID() + "_connection_slots"), arrow_(&endpoint_arrow) {

    setAlwaysOnTop(true);
    setInterceptsMouseClicks(false, true);

    for (int index = 0; index < kMaxVisibleSlots; ++index) {
        const auto prefix = getName() + "_" + juce::String(index);
        initialiseSlot(index, prefix);
    }

    arrow_->setConnectionSlots(this);
}

ConnectionSlots::ConnectionSlots (SynthSlider& destination) : SynthSection(destination.getComponentID() + "_connection_slots"), destination_(&destination) {
    setInterceptsMouseClicks(false, true);

    for (int index = 0; index < SynthSlider::kNumSlots; ++index) {
        const auto prefix = destination_->getComponentID() + "_modulation_slot_" + juce::String(index);
        initialiseSlot (index, prefix);

    }
}

ConnectionSlots::~ConnectionSlots() {
    if (arrow_ != nullptr) {
        if (arrow_->getConnectionSlots() == this)
            arrow_->setConnectionSlots(nullptr);
    }

    if (destination_ != nullptr) {
        for (int slot = 0; slot < SynthSlider::kNumSlots; ++slot)
            destination_->setExtraModulationTarget(slot, nullptr);
    }
}

void ConnectionSlots::setConnections(std::vector<ConnectionSlotData> connections) {
    const int visible_count = juce::jmin(static_cast<int>(connections.size()), kMaxVisibleSlots);

    for (int index = 0; index < kMaxVisibleSlots; ++index) {
        if (index < visible_count)
            slot_components_[index]->setConnection(std::move(connections[index]));
        else
            slot_components_[index]->clearConnection();
    }

    setVisible(visible_count > 0);
    syncOpenGl();
}

void ConnectionSlots::resized() {
    SynthSection::resized();

    if (arrow_ != nullptr) {
        const bool is_input = arrow_->getEndpoint().address.direction == electrosynth::EndpointDirection::Destination;
        const int slot_y = (getHeight() - kSlotHeight) / 2;

        for (int index = 0; index < kMaxVisibleSlots; ++index) {
            const int slot_x = is_input ? index * kSlotPitch : getWidth() - kSlotWidth - index * kSlotPitch;
            const juce::Rectangle<int> bounds { slot_x, slot_y, kSlotWidth, kSlotHeight };

            visuals_[index].body->setBounds(bounds);
            visuals_[index].border->setBounds(bounds);
            visuals_[index].label->setBounds(bounds.reduced(2, 1));
            visuals_[index].amount->setBounds(bounds.reduced(1));
            slot_components_[index]->setBounds(bounds);
        }

        syncOpenGl();
        return;
    }

    if (destination_ == nullptr)
        return;

    const auto bounds = getLocalBounds();
    for (int slot = 0; slot < SynthSlider::kNumSlots; ++slot) {
        const int left = slot * bounds.getWidth() / SynthSlider::kNumSlots;
        const int right = (slot + 1) * bounds.getWidth() / SynthSlider::kNumSlots;
        const juce::Rectangle<int> slot_bounds(left, 0, right - left, bounds.getHeight());

        slot_components_[slot]->setBounds(slot_bounds);
        slot_components_[slot]->setVisible(true);

        auto& visual = visuals_[slot];
        visual.body->setBounds(slot_bounds);
        visual.border->setBounds(slot_bounds);
        visual.label->setBounds(slot_bounds.reduced(2, 1));

        const auto aux_bounds = getAuxSlotBounds(slot_bounds);
        visual.aux_body->setBounds(aux_bounds);
        visual.aux_border->setBounds(aux_bounds);
        visual.aux_label->setBounds(aux_bounds.reduced(1, 0));
        visual.amount->setBounds(slot_bounds.reduced(1));
    }
    syncOpenGl();
}

void ConnectionSlots::syncOpenGl() {
    if (arrow_ != nullptr) {
        const auto rounding = juce::jmin(findValue(Skin::kWidgetRoundedCorner),
        static_cast<float>(kSlotHeight) * 0.5f);

        for (int index = 0; index < kMaxVisibleSlots; ++index) {
            auto* connection = slot_components_[index]->getConnection();
            const bool occupied = connection != nullptr;
            const auto colour = occupied ? connection->colour : juce::Colours::transparentBlack;

            auto& visual = visuals_[index];

            visual.body->setColor(colour.withAlpha(0.28f));
            visual.body->setRounding(rounding);
            visual.border->setColor(colour);
            visual.border->setRounding(rounding);
            visual.label->setText(occupied ? connection->label : "");
            visual.label->setTextSize(9.0f);
            visual.label->setColor(colour);

            visual.body->setVisible(occupied);
            visual.border->setVisible(occupied);
            visual.label->setVisible(occupied);

            const float amountPercent = connection != nullptr ? connection->bipolar ?
            (juce::jlimit(-1.0f, 1.0f, connection->amount) + 1.0f) * 0.5f
            : juce::jlimit(0.0f, 1.0f, connection->amount) : 0.0f;

            auto amountBounds = slot_components_[index]->getBounds().reduced(1);
            amountBounds.setWidth(juce::roundToInt(amountBounds.getWidth() * amountPercent));

            visual.amount->setBounds(amountBounds);
            visual.amount->setColor(colour.withAlpha(0.2f));
            visual.amount->setVisible(occupied && connection->hasAmount);
        }
    }

    if (destination_ == nullptr) return;

    const auto empty = juce::Colours::transparentBlack;

    for (int slot = 0; slot < SynthSlider::kNumSlots; ++slot) {
        auto* slotComponent = slot_components_[slot].get();
        const auto* connection = slotComponent->getConnection();
        bool occupied = connection != nullptr;

        auto& visuals = visuals_[slot];
        const auto color = occupied ? getBypassAdjustedColor(connection->colour, connection->bypass) : empty;
        const float amount = occupied && connection->hasAmount ?
            juce::jlimit(-1.0f, 1.0f, connection->amount) : 1.0f;
        const auto slot_bounds = slotComponent->getBounds();

        visuals.body->setColor(color.withAlpha(0.28f));
        visuals.body->setVisible(occupied);
        visuals.border->setColor(color);
        visuals.border->setVisible(true);
        visuals.label->setText(occupied ? connection->label : "");
        visuals.label->setTextSize(std::max(9.0f, slot_bounds.getHeight() * 0.45f));
        visuals.label->setColor(color);
        visuals.label->setVisible(occupied);

        visuals.amount->setColor(color.withAlpha(0.2f));
        visuals.amount->setVisible(occupied);
        auto amount_bounds = slot_bounds.reduced(1);
        amount_bounds.setWidth(std::max(0, static_cast<int>(std::round(amount_bounds.getWidth() * (amount + 1.0f) * 0.5f))));
        visuals.amount->setBounds(amount_bounds);

        const auto* auxiliary = occupied && connection->auxiliary ?
            &*connection->auxiliary : nullptr;
        const bool hasAux = auxiliary != nullptr;
        const auto auxColour = hasAux ? auxiliary->colour : empty;

        visuals.aux_body->setColor(auxColour.withAlpha(0.32f));
        visuals.aux_body->setVisible(hasAux);
        visuals.aux_border->setColor(auxColour);
        visuals.aux_border->setVisible(hasAux);
        visuals.aux_label->setText(hasAux ? auxiliary->label : "");
        visuals.aux_label->setTextSize(std::max(7.0f, slot_bounds.getHeight() * 0.28f));
        visuals.aux_label->setColor(auxColour);
        visuals.aux_label->setVisible(hasAux);
    }
}

void ConnectionSlots::slotClicked(int index, const juce::MouseEvent& event) {
    const auto* connection = slot_components_[index]->getConnection();

    if (connection == nullptr)
        return;

    for (auto* listener : listeners_)
        listener->connectionSlotClicked(*connection, event);
}

void ConnectionSlots::slotAmountChanged(int index, float amount) {
    const auto* connection = slot_components_[index]->getConnection();
    if (connection == nullptr)
        return;

    for (auto* listener : listeners_)
        listener->connectionAmountChanged(*connection, amount);
}

void ConnectionSlots::addListener(Listener* listener) {
    if (listener != nullptr && std::find(listeners_.begin(), listeners_.end(), listener) == listeners_.end()) {
        listeners_.push_back(listener);
    }
}

void ConnectionSlots::removeListener(Listener* listener) {
    std::erase(listeners_, listener);
}
