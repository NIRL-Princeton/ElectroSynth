//
// Created by Callista Chong on 7/22/26.
//


#include "modulation_slots.h"
#include <cmath>

namespace electrosynth {

    namespace {
        juce::Rectangle<int> getAuxSlotBounds(juce::Rectangle<int> slot_bounds) {
            auto aux_bounds = slot_bounds.reduced(2, 2);
            aux_bounds.setTop(slot_bounds.getBottom());
            return aux_bounds;
        }

        juce::Colour getBypassAdjustedColor(juce::Colour color, bool bypass) {
            return bypass ? color.withSaturation(0.0f).withMultipliedBrightness(0.85f) : color;
        }
    } // namespace

    ModulationSlotComponent::ModulationSlotComponent(SynthSlider& destination_slider, int slot_index)
        : destination_slider_(destination_slider), slot_index_(slot_index) {

        jassert(juce::isPositiveAndBelow(slot_index_, SynthSlider::kNumModulationSlots));
        setComponentID(destination_slider_.getComponentID() + "_modulation_slot_" + juce::String(slot_index_));
        setInterceptsMouseClicks(false, false);
    }

    void ModulationSlotComponent::paint(juce::Graphics&) { }

    void ModulationSlotComponent::notifySlotHost() {
        repaint();
        if (auto* slots = findParentComponentOfClass<ModulationSlots>())
            slots->syncOpenGl();
    }

    void ModulationSlotComponent::setSourceName(juce::String source_name) {
        if (source_name_ == source_name)
            return;
        source_name_ = std::move(source_name);
        notifySlotHost();
    }

    void ModulationSlotComponent::setSourceDisplayLabel(juce::String display_label) {
        if (display_label_ == display_label)
            return;
        display_label_ = std::move(display_label);
        notifySlotHost();
    }

    void ModulationSlotComponent::setModulationAmount(float amount) {
        amount = juce::jlimit(-1.0f, 1.0f, amount);
        if (juce::approximatelyEqual(modulation_amount_, amount))
            return;
        modulation_amount_ = amount;
        notifySlotHost();
    }

    void ModulationSlotComponent::setBypass(bool bypass) {
        if (bypass_ == bypass)
            return;
        bypass_ = bypass;
        notifySlotHost();
    }

    void ModulationSlotComponent::setAuxSource(juce::String source_name, juce::String display_label) {
        if (aux_source_name_ == source_name && aux_display_label_ == display_label)
            return;
        aux_source_name_ = std::move(source_name);
        aux_display_label_ = std::move(display_label);
        notifySlotHost();
    }

    void ModulationSlotComponent::clearSource() {
        const bool changed = source_name_.isNotEmpty() || display_label_.isNotEmpty()
                         || aux_source_name_.isNotEmpty() || aux_display_label_.isNotEmpty()
                         || !juce::approximatelyEqual(modulation_amount_, 0.0f) || bypass_;
        source_name_.clear();
        display_label_.clear();
        aux_source_name_.clear();
        aux_display_label_.clear();
        modulation_amount_ = 0.0f;
        bypass_ = false;
        if (changed)
            notifySlotHost();
    }

    juce::Colour ModulationSlotComponent::getColorForSource(const juce::String& source_name) const {
        if (source_name.startsWithIgnoreCase("env"))
            return findColour(Skin::kEnvelopeAccent);
        if (source_name.startsWithIgnoreCase("lfo"))
            return findColour(Skin::kLFOAccent);
        if (source_name.startsWithIgnoreCase("vca") || source_name.containsIgnoreCase("master"))
            return findColour(Skin::kMasterEnvelopeAccent);
        return ShaderColors::kNoise;
    }

    juce::String ModulationSlotComponent::getLabelForSource(const juce::String& source_name,
                                                        const juce::String& display_label) const {
        if (display_label.isNotEmpty())
            return display_label;

        juce::String prefix;
        if (source_name.startsWithIgnoreCase("env"))
            prefix = "Env ";
        else if (source_name.startsWithIgnoreCase("lfo"))
            prefix = "Lfo ";
        else if (source_name.startsWithIgnoreCase("vca") || source_name.containsIgnoreCase("master"))
            prefix = "Master ";
        else
            return source_name;

        juce::String digits;
        for (auto character : source_name)
            if (juce::CharacterFunctions::isDigit(character))
                digits += character;
        return prefix + (digits.isNotEmpty() ? digits : "");
    }

    juce::Colour ModulationSlotComponent::getSourceColor() const {
        return getColorForSource(source_name_);
    }

    juce::String ModulationSlotComponent::getSourceLabel() const {
        return getLabelForSource(source_name_, display_label_);
    }

    juce::Colour ModulationSlotComponent::getAuxSourceColor() const {
        return getColorForSource(aux_source_name_);
    }

    juce::String ModulationSlotComponent::getAuxSourceLabel() const {
        return getLabelForSource(aux_source_name_, aux_display_label_);
    }

    ModulationSlots::ModulationSlots(SynthSlider& destination)
        : SynthSection(destination.getComponentID() + "_modulation_slots"), destination_(destination) {

        setInterceptsMouseClicks(false, true);

        for (int slot = 0; slot < SynthSlider::kNumModulationSlots; ++slot) {
            auto target = std::make_unique<ModulationSlotComponent>(destination_, slot);
            addAndMakeVisible(target.get());
            destination_.setExtraModulationTarget(slot, target.get());
            slots_[slot] = std::move(target);

            auto& visuals = visuals_[slot];
            const auto prefix = destination_.getComponentID() + "_modulation_slot_" + juce::String(slot);
            visuals.body = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, prefix + "_body");
            visuals.amount = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, prefix + "_amount");
            visuals.border = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleBorderFragment, prefix + "_border");
            visuals.label = std::make_shared<PlainTextComponent>(prefix + "_label", "");
            visuals.aux_body = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, prefix + "_aux_body");
            visuals.aux_border = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleBorderFragment,
                                                         prefix + "_aux_border");
            visuals.aux_label = std::make_shared<PlainTextComponent>(prefix + "_aux_label", "");

            for (auto* quad : { visuals.body.get(), visuals.amount.get(), visuals.border.get(),
                            visuals.aux_body.get(), visuals.aux_border.get() }) {
                quad->setInterceptsMouseClicks(false, false);
                quad->setAlwaysOnTop(true);
                            }
            for (auto* label : { visuals.label.get(), visuals.aux_label.get() }) {
                label->setInterceptsMouseClicks(false, false);
                label->setAlwaysOnTop(true);
                label->setFontType(PlainTextComponent::kRegular);
                label->setJustification(juce::Justification::centred);
            }

            visuals.border->setThickness(1.0f, true);
            visuals.aux_border->setThickness(1.0f, true);

            addOpenGlComponent(visuals.body);
            addOpenGlComponent(visuals.amount);
            addOpenGlComponent(visuals.border);
            addOpenGlComponent(visuals.label);
            addOpenGlComponent(visuals.aux_body);
            addOpenGlComponent(visuals.aux_border);
            addOpenGlComponent(visuals.aux_label);
        }
    }

    ModulationSlots::~ModulationSlots() {
        for (int slot = 0; slot < SynthSlider::kNumModulationSlots; ++slot)
            destination_.setExtraModulationTarget(slot, nullptr);
    }

    void ModulationSlots::resized() {
        const auto bounds = getLocalBounds();
        for (int slot = 0; slot < SynthSlider::kNumModulationSlots; ++slot) {
            const int left = slot * bounds.getWidth() / SynthSlider::kNumModulationSlots;
            const int right = (slot + 1) * bounds.getWidth() / SynthSlider::kNumModulationSlots;
            const juce::Rectangle<int> slot_bounds(left, 0, right - left, bounds.getHeight());
            slots_[slot]->setBounds(slot_bounds);
            slots_[slot]->setVisible(true);

            auto& visuals = visuals_[slot];
            visuals.body->setBounds(slot_bounds);
            visuals.border->setBounds(slot_bounds);
            visuals.label->setBounds(slot_bounds.reduced(2, 1));
            const auto aux_bounds = getAuxSlotBounds(slot_bounds);
            visuals.aux_body->setBounds(aux_bounds);
            visuals.aux_border->setBounds(aux_bounds);
            visuals.aux_label->setBounds(aux_bounds.reduced(1, 0));
            visuals.amount->setBounds(slot_bounds.reduced(1));
        }
        syncOpenGl();
    }

    void ModulationSlots::syncOpenGl() {
        const auto empty = juce::Colours::transparentBlack;
        for (int slot = 0; slot < SynthSlider::kNumModulationSlots; ++slot) {
            auto* state = slots_[slot].get();
            auto& visuals = visuals_[slot];
            const bool occupied = state->isOccupied();
            const auto color = occupied ? getBypassAdjustedColor(state->getSourceColor(), state->isBypass()) : empty;
            const bool has_aux = state->hasAuxSource();
            const auto aux_color = has_aux ? state->getAuxSourceColor() : empty;
            const float amount = juce::jlimit(-1.0f, 1.0f, state->getModulationAmount());
            const auto slot_bounds = state->getBounds();

            visuals.body->setColor(color.withAlpha(0.28f));
            visuals.body->setVisible(occupied);
            visuals.border->setColor(color);
            visuals.border->setVisible(true);
            visuals.label->setText(occupied ? state->getSourceLabel() : "");
            visuals.label->setTextSize(std::max(9.0f, slot_bounds.getHeight() * 0.45f));
            visuals.label->setColor(color);
            visuals.label->setVisible(occupied);

            visuals.amount->setColor(color.withAlpha(0.2f));
            visuals.amount->setVisible(occupied);
            auto amount_bounds = slot_bounds.reduced(1);
            amount_bounds.setWidth(std::max(0, static_cast<int>(
                std::round(amount_bounds.getWidth() * (amount + 1.0f) * 0.5f))));
            visuals.amount->setBounds(amount_bounds);

            visuals.aux_body->setColor(aux_color.withAlpha(0.32f));
            visuals.aux_body->setVisible(occupied && has_aux);
            visuals.aux_border->setColor(aux_color);
            visuals.aux_border->setVisible(occupied && has_aux);
            visuals.aux_label->setText(has_aux ? state->getAuxSourceLabel() : "");
            visuals.aux_label->setTextSize(std::max(7.0f, slot_bounds.getHeight() * 0.28f));
            visuals.aux_label->setColor(aux_color);
            visuals.aux_label->setVisible(occupied && has_aux);
        }
    }
} // namespace electrosynth
