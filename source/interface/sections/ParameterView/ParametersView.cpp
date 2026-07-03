#include "ParametersView.h"
#include "FxModuleTemplateView.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "open_gl_background.h"
#include "open_gl_combobox.h"

// ParametersView.cpp is the generic parameter-to-controller builder. Given a processor’s parameter list, it creates the right UI component
// for each parameter:
// BoolParameter -> toggle button
// ChoiceParameter -> combo box
// FloatParameter -> SynthSlider
// Each class is a small wrapper object that makes the UI component, attaches it to the parameter, and registers it with the SynthSection parent

namespace electrosynth {

    namespace {
        juce::Colour getModulationSlotSourceColor(const juce::String& source_name) {
            if (source_name.startsWithIgnoreCase("env"))
                return ShaderColors::kEnvelopeTextColor;
            if (source_name.startsWithIgnoreCase("lfo"))
                return ShaderColors::kLfoTextColor;
            if (source_name.startsWithIgnoreCase("vca")
                || source_name.containsIgnoreCase("master"))
                return ShaderColors::kMasterEnvelopeTextColor;
            return ShaderColors::kSoundModuleTextColor;
        }

        juce::String getModulationSlotSourceLabel(const juce::String& source_name,
                                                  const juce::String& display_label) {
            if (display_label.isNotEmpty())
                return display_label;

            juce::String prefix;
            if (source_name.startsWithIgnoreCase("env")) prefix = "Env ";
            else if (source_name.startsWithIgnoreCase("lfo")) prefix = "Lfo ";
            else if (source_name.startsWithIgnoreCase("vca") || source_name.containsIgnoreCase("master"))
                prefix = "Master ";
            else
                return source_name;

            juce::String digits;
            for (auto character : source_name) {
                if (juce::CharacterFunctions::isDigit(character))
                    digits += character;
            }

            return prefix + (digits.isNotEmpty() ? digits : "");
        }

        juce::Rectangle<float> getAuxSlotBounds(juce::Rectangle<float> slot_bounds) {
            auto aux_bounds = slot_bounds.reduced(2.0f, 2.0f);
            aux_bounds.setTop(slot_bounds.getCentreY());
            return aux_bounds;
        }
    }

	ModulationSlotComponent::ModulationSlotComponent(SynthSlider& destination_slider, int slot_index)
    : destination_slider_(destination_slider), slot_index_(slot_index) {

    jassert(juce::isPositiveAndBelow(slot_index_, SynthSlider::kNumModulationSlots));
    setComponentID(destination_slider_.getComponentID() + "_modulation_slot_" + juce::String(slot_index_));
    setInterceptsMouseClicks(false, false);
}

void ModulationSlotComponent::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();
    const auto empty_border_color = juce::Colour::fromRGB(54, 78, 79);

    if (isOccupied()) {
        const auto source_color = getSourceColor();
        const auto amount = juce::jlimit(0.0f, 1.0f, modulation_amount_);

        g.setColour(source_color.withAlpha(0.28f));
        g.fillRect(bounds.reduced(1.0f));

        const float meter_thickness = std::max(2.0f, bounds.getHeight() * 0.12f);
        const float meter_width = std::max(0.0f, bounds.getWidth() * amount - 2.0f);
        g.setColour(source_color.withAlpha(0.45f));
        g.fillRect(bounds.getX() + 1.0f, bounds.getBottom() - meter_thickness - 1.0f, meter_width, meter_thickness);

        g.setColour(source_color);
        g.setFont(juce::Font(std::max(9.0f, bounds.getHeight() * 0.45f), juce::Font::plain));
        g.drawFittedText(getSourceLabel(), getLocalBounds().reduced(2, 1),
                         juce::Justification::centred, 1);

        if (hasAuxSource()) {
            const auto aux_bounds = getAuxSlotBounds(bounds);
            const auto aux_color = getAuxSourceColor();
            g.setColour(aux_color.withAlpha(0.32f));
            g.fillRect(aux_bounds);
            g.setColour(aux_color);
            g.drawRect(aux_bounds, 1.0f);
            g.setFont(juce::Font(std::max(7.0f, aux_bounds.getHeight() * 0.55f), juce::Font::plain));
            g.drawFittedText(getAuxSourceLabel(), aux_bounds.toNearestInt().reduced(1, 0),
                             juce::Justification::centred, 1);
        }
    }
    else {
        g.setColour(empty_border_color);
    }

    g.drawRect(bounds.reduced(0.5f), 1.0f);
}

void ModulationSlotComponent::setSourceName(juce::String source_name) {
    if (source_name_ == source_name)
        return;

    source_name_ = std::move(source_name);
    repaint();
    if (auto* parameters_view = findParentComponentOfClass<ParametersView>()) {
        parameters_view->syncModulationSlotOpenGl();
        parameters_view->repaintBackground();
    }
}

void ModulationSlotComponent::setSourceDisplayLabel(juce::String display_label) {
    if (display_label_ == display_label)
        return;

    display_label_ = std::move(display_label);
    repaint();
    if (auto* parameters_view = findParentComponentOfClass<ParametersView>()) {
        parameters_view->syncModulationSlotOpenGl();
        parameters_view->repaintBackground();
    }
}

void ModulationSlotComponent::setModulationAmount(float amount)
{
    amount = juce::jlimit(0.0f, 1.0f, amount);
    if (juce::approximatelyEqual(modulation_amount_, amount))
        return;

    modulation_amount_ = amount;
    repaint();
    if (auto* parameters_view = findParentComponentOfClass<ParametersView>()) {
        parameters_view->syncModulationSlotOpenGl();
        parameters_view->repaintBackground();
    }
}

juce::Colour ModulationSlotComponent::getSourceColor() const {
    return getModulationSlotSourceColor(source_name_);
}

juce::String ModulationSlotComponent::getSourceLabel() const { // for marking connections in the boxes underneath knobs
    return getModulationSlotSourceLabel(source_name_, display_label_);
}

void ModulationSlotComponent::setAuxSource(juce::String source_name, juce::String display_label) {
    if (aux_source_name_ == source_name && aux_display_label_ == display_label)
        return;

    aux_source_name_ = std::move(source_name);
    aux_display_label_ = std::move(display_label);
    repaint();
    if (auto* parameters_view = findParentComponentOfClass<ParametersView>()) {
        parameters_view->syncModulationSlotOpenGl();
        parameters_view->repaintBackground();
    }
}

juce::Colour ModulationSlotComponent::getAuxSourceColor() const {
    return getModulationSlotSourceColor(aux_source_name_);
}

juce::String ModulationSlotComponent::getAuxSourceLabel() const {
    return getModulationSlotSourceLabel(aux_source_name_, aux_display_label_);
}

// Rotary slider that suppresses the value bubble popup on drag/hover.
class NoPopupSynthSlider : public SynthSlider {
public:
    using SynthSlider::SynthSlider;
    bool shouldShowPopup() override { return false; }
    // Hide both arc segments; thumb indicator (kRotaryHand) still renders.
    juce::Colour getSelectedColor()   const override { return juce::Colours::transparentBlack; }
    juce::Colour getUnselectedColor() const override { return juce::Colours::transparentBlack; }
    // Halve indicator and suppress hover-boost by pre-dividing when dragging.
    // redoImage() multiplies kKnobArcThickness by 1.4 on hover; dividing here cancels that.
    float findValue(Skin::ValueId value_id) const override {
        float base = SynthSlider::findValue(value_id);
        if (value_id == Skin::kKnobArcThickness)
            return base * (isMouseOverOrDragging() ? (0.5f / 1.4f) : 0.5f);
        return base;
    }
};

    namespace parameters_view_detail {

        //==============================================================================
        class BooleanParameterComponent : public juce::Component {
        public:
            BooleanParameterComponent(chowdsp::BoolParameter &param, chowdsp::PluginState& listeners,SynthSection &parent)
                    : button(param.name), attachment(param, listeners, button) {
                button.setComponentID(param.paramID);
                setLookAndFeel(DefaultLookAndFeel::instance());
                parent.addButton(&button);
            }

            void resized() override {
                auto area = getBoundsInParent();
                button.setBounds(area);
            }

        private:
            OpenGlToggleButton button;
            chowdsp::ButtonAttachment attachment;
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BooleanParameterComponent)
        };

        class ChoiceParameterComponent : public juce::Component {
        public:
            ChoiceParameterComponent(chowdsp::ChoiceParameter &param, chowdsp::PluginState& listeners,SynthSection &parent)
                    : attachment(param, listeners, box) {
                addAndMakeVisible(box);
                parent.addChildComponent (box);
                parent.addOpenGlComponent (box.getImageComponent());
            }

            void resized() override {
                auto area = getBoundsInParent();
                area.removeFromLeft(8);
                box.setBounds(area.reduced(0, 10));
            }

        private:
            OpenGLComboBox box;
            chowdsp::ComboBoxAttachment attachment;
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoiceParameterComponent)
        };

        class SliderParameterComponent : public juce::Component {
        public:
            SliderParameterComponent(chowdsp::FloatParameter &param, chowdsp::PluginState& listeners, SynthSection &parent)
                    : slider(param.name), attachment(param, listeners, slider) {
                slider.setComponentID(param.paramID);
                setLookAndFeel(DefaultLookAndFeel::instance());
                slider.setScrollWheelEnabled(false);
                slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                //setInterceptsMouseClicks(false, true);
                parent.addSlider(&slider, true);
                slider.parentHierarchyChanged();

                _ASSERT(slider.getSectionParent() != nullptr);
                DBG("create slider for " + param.paramID + "with parent " + parent.getName());
             }

             ~SliderParameterComponent() {
//                auto parent = findParentComponentOfClass<SynthGuiInterface>();
//                parent->getOpenGlWrapper()
             }

             void mouseEnter (const MouseEvent& event) {
                 DBG("mouseentersliderparamacomp");
             }

            void resized() override {
                auto area = getBoundsInParent();
                slider.setBounds(area);
                slider.redoImage();
            }

        private:
            SynthSlider slider;
            chowdsp::SliderAttachment attachment;
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SliderParameterComponent)
        };

        std::unique_ptr<juce::Component> createParameterComp(chowdsp::PluginState& listeners, juce::RangedAudioParameter &parameter, SynthSection& parent) {
            if (auto *boolParam = dynamic_cast<chowdsp::BoolParameter *> (&parameter))
                return std::make_unique<BooleanParameterComponent>(*boolParam, listeners,parent);

            if (auto *choiceParam = dynamic_cast<chowdsp::ChoiceParameter *> (&parameter))
                return std::make_unique<ChoiceParameterComponent>(*choiceParam, listeners,parent);

            if (auto *sliderParam = dynamic_cast<chowdsp::FloatParameter *> (&parameter))
                return std::make_unique<SliderParameterComponent>(*sliderParam, listeners, parent);

            return {};
        }
//        struct ParameterGroupItem : public SynthSection {
//            ParameterGroupItem(chowdsp::ParamHolder &params, chowdsp::ParameterListeners& listeners, SynthSection &parent)
//                    : name(params.getName()), parent(parent), label(name,name), SynthSection(params.getName()) {
//                setLookAndFeel(DefaultLookAndFeel::instance());
//                params.doForAllParameterContainers(
//                        [this, &listeners](auto &paramVec) {
//                            for (auto &param: paramVec)
//                            {
//                                comps.push_back(createParameterComp(listeners, param,*this));
//                                addAndMakeVisible(comps.back().get());
//                            }
//
//                        },
//                        [this, &listeners](auto &paramHolder) {
//                            DBG("add group item");
//                            addSubSection(std::make_unique<ParameterGroupItem>(paramHolder,listeners, *this).release());
//                        });
//
//            }
//
//
//            void resized() override
//            {
////                int widget_margin = findValue(Skin::kWidgetMargin);
////                int title_width = getTitleWidth();
////                int section_height = getKnobSectionHeight();
////                DBG("--------ogroupitem"  + name +" View -------------");
////                DBG("bounds x:" + juce::String(getLocalBounds().getX()) + " y:" + juce::String(getLocalBounds().getY()) + " width: " + juce::String(getLocalBounds().getWidth()) + " height: " + juce::String(getLocalBounds().getHeight()));
//////        pimpl->view.setBounds(getLocalBounds());
//////                juce::Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
//////                int i = 0;
//////                for(auto section : sub_sections_)
//////                {
//////                    juce::Rectangle<int> section_area = getDividedAreaBuffered(bounds, sub_sections_.size(), i++, widget_margin);
//////                    section->setBounds(section_area);
//////                }
////                int editor_x = getLocalBounds().getX();
////                int editor_width = getLocalBounds().getWidth();
////                int knob_y2 = section_height - widget_margin;
////                juce::Rectangle<int> knobs_area = getDividedAreaBuffered(getLocalBounds(), 3, 0, widget_margin);
////                placeKnobsInArea(getLocalBounds(),
////                comps) ;
////
//
//            }
//             juce::String getUniqueName() const
//             {
//                return name;
//             }
//             std::vector<std::unique_ptr<juce::Component>> comps;
//            SynthSection &parent;
//            juce::String name;
//            juce::Grid grid;
//            juce::Label label;
//        };
    } // namespace parameters_view_detail

//==============================================================================
//    struct ParametersView::Pimpl {
//        Pimpl(chowdsp::ParamHolder &params, chowdsp::ParameterListeners& listeners,SynthSection& parent)
//                :   groupItem(params, listeners, parent){
//            //const auto numIndents = getNumIndents(groupItem);
//            //const auto width = 400 + view.getIndentSize() * numIndents;
//
//            //view.setSize(width, 600);
//            //view.setDefaultOpenness(true);
//            //view.setRootItemVisible(false);
//
//        }
//
////        parameters_view_detail::ParameterGroupItem groupItem;
//        juce::Grid parameterGrid;
//    };

//==============================================================================
    ParametersView::ParametersView(chowdsp::PluginState &pluginState, chowdsp::ParamHolder &params, String name)
            // : ParametersView (pluginState.getParameterListeners(), params, name) {:
        : SynthSection(name)
    {
        setComponentID(name);
        setInterceptsMouseClicks(false,true);

        params.doForAllParameterContainers(
            [this, &pluginState](auto &paramVec) {
                    for (auto &param: paramVec) {
                        comps.push_back(parameters_view_detail::createParameterComp(pluginState, param, *this));
                    }
                },
                [this, &pluginState](auto &paramHolder) {
                   // DBG("add group item");
                   // addSubSection(std::make_unique<ParameterGroupItem>(paramHolder,listeners, *this).release());
	                });
        setLookAndFeel(DefaultLookAndFeel::instance());
        setOpaque(true);
        ensureSliderLabels();
    }

    ParametersView::ParametersView(chowdsp::ParameterListeners& paramListeners, chowdsp::ParamHolder& params, String name)
        :  SynthSection(name)
    {
        setComponentID(name);
        setInterceptsMouseClicks(false,true);
              //pimpl(std::make_unique<Pimpl>(params, paramListeners, *this)){
//        auto *viewport = pimpl->view.getViewport();
//         params.doForAllParameterContainers(
//                 [this, &paramListeners](auto &paramVec) {
//                     for (auto &param: paramVec)
//                     {
//                         comps.push_back(parameters_view_detail::createParameterComp(paramListeners, param,*this));
//
//                     }
//                 },
//                 [this, &paramListeners](auto &paramHolder) {
//                    // DBG("add group item");
// //                    addSubSection(std::make_unique<ParameterGroupItem>(paramHolder,listeners, *this).release());
//                 });
//         setLookAndFeel(DefaultLookAndFeel::instance());
//         setOpaque(true);
// //        addAndMakeVisible(pimpl->view);
// //        viewport->setScrollBarsShown (true, false);
// //        setSize(viewport->getViewedComponent()->getWidth() + viewport->getVerticalScrollBar().getWidth(),
// //                juce::jlimit(125, 700, viewport->getViewedComponent()->getHeight()));
    }
    ParametersView::~ParametersView() = default;

    int ParametersView::getKnobsPerRow() const
    {
        return getName().startsWithIgnoreCase("string") ? kStringKnobsPerRow : kDefaultKnobsPerRow;
    }

    int ParametersView::getKnobRowCount() const {
        if (all_sliders_v.empty())
            return 1;

        return static_cast<int> (std::ceil (all_sliders_v.size() / static_cast<float> (getKnobsPerRow())));
    }

    int ParametersView::getPreferredHeight() const {
        return getKnobRowCount() * kModuleHeightPerKnobRow;
    }

    juce::Colour ParametersView::getSliderLabelColor() const {
        if (getName().startsWithIgnoreCase("env"))
            return ShaderColors::kEnvelopeTextColor;
        if (getName().startsWithIgnoreCase("lfo"))
            return ShaderColors::kLfoTextColor;
        if (getName().equalsIgnoreCase("VCA")
            || getName().containsIgnoreCase("master"))
            return ShaderColors::kMasterEnvelopeTextColor;

        return ShaderColors::kSoundModuleTextColor;
    }

    void ParametersView::paint(juce::Graphics &g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void ParametersView::paintBackground(juce::Graphics& g) {
        SynthSection::paintContainer(g);
        paintBorder(g);
        paintKnobShadows(g);
        paintChildrenBackgrounds(g);

        const auto empty_border_color = juce::Colour::fromRGB(54, 78, 79);

        // paint three box targets below the knob
        for (const auto& [slider, bounds] : modulation_boxes_) {
            if (slider == nullptr || !slider->isVisible())
                continue;

            auto slots = modulation_box_targets_.find(slider);
            if (slots == modulation_box_targets_.end())
                continue;

            for (const auto& slot : slots->second) {
                if (slot == nullptr)
                    continue;

                auto slot_bounds = slot->getBounds().toFloat();
                if (slot->isOccupied()) {
                    const auto source_color = slot->getSourceColor();
                    const auto amount = juce::jlimit(0.0f, 1.0f, slot->getModulationAmount());

                    g.setColour(source_color.withAlpha(0.28f));
                    g.fillRect(slot_bounds.reduced(1.0f));

                    const float meter_thickness = std::max(2.0f, slot_bounds.getHeight() * 0.12f);
                    const float meter_width = std::max(0.0f, slot_bounds.getWidth() * amount - 2.0f);
                    g.setColour(source_color.withAlpha(0.45f));
                    g.fillRect(slot_bounds.getX() + 1.0f,
                               slot_bounds.getBottom() - meter_thickness - 1.0f,
                               meter_width,
                               meter_thickness);

	                    g.setColour(source_color);
	                    g.setFont(juce::Font(std::max(9.0f, slot_bounds.getHeight() * 0.45f), juce::Font::bold));
	                    g.drawFittedText(slot->getSourceLabel(), slot->getBounds().reduced(2, 1),
	                                     juce::Justification::centred, 1);

	                    if (slot->hasAuxSource()) {
	                        const auto aux_bounds = getAuxSlotBounds(slot_bounds);
	                        const auto aux_color = slot->getAuxSourceColor();
	                        g.setColour(aux_color.withAlpha(0.32f));
	                        g.fillRect(aux_bounds);
	                        g.setColour(aux_color);
	                        g.drawRect(aux_bounds, 1.0f);
	                        g.setFont(juce::Font(std::max(7.0f, aux_bounds.getHeight() * 0.55f), juce::Font::bold));
	                        g.drawFittedText(slot->getAuxSourceLabel(), aux_bounds.toNearestInt().reduced(1, 0),
	                                         juce::Justification::centred, 1);
	                    }
	                }
                else {
                    g.setColour(empty_border_color);
                }

                g.drawRect(slot_bounds, 1.0f);
            }
        }
    }

    void ParametersView::ensureSliderLabels() {
        for (auto slider : all_sliders_v) {
            if (slider_labels_.count(slider) != 0)
                continue;

            auto label = std::make_shared<PlainTextComponent>(slider->getName() + "_label", slider->getName());
            label->setFontType(PlainTextComponent::kRegular);
            label->setJustification(juce::Justification::centred);
            label->setColor(getSliderLabelColor());
            addOpenGlComponent(label);
            slider_labels_[slider] = label;

            if (auto* synth_slider = dynamic_cast<SynthSlider*>(slider)) {
                ModulationSlots slots;
                ModulationSlotOpenGlSet open_gl_slots;
                for (int slot_index = 0; slot_index < SynthSlider::kNumModulationSlots; ++slot_index) {
                    auto target = std::make_unique<ModulationSlotComponent>(*synth_slider, slot_index);
                    addAndMakeVisible(target.get());
                    synth_slider->setExtraModulationTarget(slot_index, target.get());
                    slots[slot_index] = std::move(target);

                    auto body = std::make_shared<OpenGlQuad>(
                        Shaders::kColorFragment,
                        synth_slider->getComponentID() + "_modulation_slot_body_" + juce::String(slot_index));
                    auto amount = std::make_shared<OpenGlQuad>(
                        Shaders::kColorFragment,
                        synth_slider->getComponentID() + "_modulation_slot_amount_" + juce::String(slot_index));
                    auto border = std::make_shared<OpenGlQuad>(
                        Shaders::kRoundedRectangleBorderFragment,
                        synth_slider->getComponentID() + "_modulation_slot_border_" + juce::String(slot_index));
	                    auto label = std::make_shared<PlainTextComponent>(
	                        synth_slider->getComponentID() + "_modulation_slot_label_" + juce::String(slot_index), "");
	                    auto aux_body = std::make_shared<OpenGlQuad>(
	                        Shaders::kColorFragment,
	                        synth_slider->getComponentID() + "_modulation_slot_aux_body_" + juce::String(slot_index));
	                    auto aux_border = std::make_shared<OpenGlQuad>(
	                        Shaders::kRoundedRectangleBorderFragment,
	                        synth_slider->getComponentID() + "_modulation_slot_aux_border_" + juce::String(slot_index));
	                    auto aux_label = std::make_shared<PlainTextComponent>(
	                        synth_slider->getComponentID() + "_modulation_slot_aux_label_" + juce::String(slot_index), "");

	                    body->setInterceptsMouseClicks(false, false);
	                    amount->setInterceptsMouseClicks(false, false);
	                    border->setInterceptsMouseClicks(false, false);
	                    label->setInterceptsMouseClicks(false, false);
	                    aux_body->setInterceptsMouseClicks(false, false);
	                    aux_border->setInterceptsMouseClicks(false, false);
	                    aux_label->setInterceptsMouseClicks(false, false);
	                    body->setAlwaysOnTop(true);
	                    amount->setAlwaysOnTop(true);
	                    border->setAlwaysOnTop(true);
	                    label->setAlwaysOnTop(true);
	                    aux_body->setAlwaysOnTop(true);
	                    aux_border->setAlwaysOnTop(true);
	                    aux_label->setAlwaysOnTop(true);
	                    border->setThickness(1.0f, true);
	                    aux_border->setThickness(1.0f, true);
	                    label->setFontType(PlainTextComponent::kRegular);
	                    label->setJustification(juce::Justification::centred);
	                    aux_label->setFontType(PlainTextComponent::kRegular);
	                    aux_label->setJustification(juce::Justification::centred);

	                    addOpenGlComponent(body);
	                    addOpenGlComponent(amount);
	                    addOpenGlComponent(border);
	                    addOpenGlComponent(label);
	                    addOpenGlComponent(aux_body);
	                    addOpenGlComponent(aux_border);
	                    addOpenGlComponent(aux_label);

	                    open_gl_slots[slot_index].body = std::move(body);
	                    open_gl_slots[slot_index].amount = std::move(amount);
	                    open_gl_slots[slot_index].border = std::move(border);
	                    open_gl_slots[slot_index].label = std::move(label);
	                    open_gl_slots[slot_index].aux_body = std::move(aux_body);
	                    open_gl_slots[slot_index].aux_border = std::move(aux_border);
	                    open_gl_slots[slot_index].aux_label = std::move(aux_label);
	                }
                modulation_box_targets_[slider] = std::move(slots);
                modulation_box_open_gl_[slider] = std::move(open_gl_slots);
            }
        }
    }

    void ParametersView::updateSliderLabels() {
        ensureSliderLabels();

        for (auto slider : all_sliders_v) {
            auto it = slider_labels_.find(slider);
            if (it == slider_labels_.end())
                continue;

            const auto bounds = slider->getBounds();
            it->second->setBounds(bounds.getX(),
                                  bounds.getY() - kKnobLabelGap - kKnobLabelHeight,
                                  bounds.getWidth(),kKnobLabelHeight);
            it->second->setText(slider->getName());
            it->second->setTextSize(getLabelFont().getHeight());
            it->second->setColor(getSliderLabelColor());
        }
    }

    void ParametersView::resized() {
        //DBG("--------" + getName() + "View -------------");
        //DBG("bounds x:" + juce::String(getLocalBounds().getX()) + " y:" + juce::String(getLocalBounds().getY()) + " width: " + juce::String(getLocalBounds().getWidth()) + " height: " + juce::String(getLocalBounds().getHeight()));
        //pimpl->groupItem.setBounds(getLocalBounds());
        ensureSliderLabels();

        // creating osc/string component bounds
        const int knobs_per_row = getKnobsPerRow();
        const int num_rows = getKnobRowCount();
        const int widget_margin = findValue(Skin::kWidgetMargin);
        const int row_height = getHeight() / num_rows;

        for (int row = 0; row < num_rows; ++row) {
            const int first = row * knobs_per_row;
            const int last = std::min<int>(first + knobs_per_row, all_sliders_v.size());
            const int count = last - first;
            if (count <= 0)
                continue;

            juce::Rectangle<int> row_area(0, row * row_height, getWidth(), row_height);
            const float component_width = (row_area.getWidth() - (count + 1) * widget_margin) / static_cast<float>(count);
            float x = row_area.getX() + widget_margin;
            int top = row_area.getY() + kKnobLabelHeight + kKnobLabelGap;
            if (vertically_center_knobs_) {
                const int rendered_knob_height = static_cast<int>(std::ceil(
                    2.0f * (findValue(Skin::kKnobArcSize)
                            + findValue(Skin::kKnobArcThickness))));
                const int content_height = kKnobLabelHeight + kKnobLabelGap
                                           + rendered_knob_height
                                           + kModulationBoxGap
                                           + kModulationBoxHeight;
                top = row_area.getY()
                      + std::max(0, (row_area.getHeight() - content_height) / 2)
                      + kKnobLabelHeight + kKnobLabelGap;
            }

            const int available_knob_height = std::max(0, row_area.getBottom() - widget_margin - kModulationBoxHeight
                                                                - kModulationBoxGap - top);

            for (int i = first; i < last; ++i) {
                const int left = std::round(x);
                const int right = std::round(x + component_width);

                if (auto* slider = dynamic_cast<SynthSlider*> (all_sliders_v[i])) {
                    const float arc_size = slider->findValue(Skin::kKnobArcSize) * slider->getKnobSizeScale();
                    const float arc_thickness = slider->findValue(Skin::kKnobArcThickness);
                    const int rendered_knob_height = static_cast<int>(std::ceil(2.0f * (arc_size + arc_thickness)));
                    const int knob_height = std::min(available_knob_height, rendered_knob_height);

                    slider->setBounds(left, top, right - left, knob_height);
                    slider->redoImage();

                    const int box_width = std::max(0, std::min(kModulationBoxWidth, right - left - 2 * widget_margin));
                    const int box_x = left + ((right - left) - box_width) / 2;
                    const juce::Rectangle<int> box_bounds { box_x, slider->getBottom() + kModulationBoxGap,
                                                            box_width, kModulationBoxHeight};
                    modulation_boxes_[slider] = box_bounds;

                    auto target = modulation_box_targets_.find(slider);
                    if (target != modulation_box_targets_.end()) {
                        for (int slot_index = 0; slot_index < SynthSlider::kNumModulationSlots; ++slot_index) {
                            const int slot_left = box_bounds.getX() + (slot_index * box_bounds.getWidth())
                                                        / SynthSlider::kNumModulationSlots;
                            const int slot_right = box_bounds.getX() + ((slot_index + 1) * box_bounds.getWidth())
                                                        / SynthSlider::kNumModulationSlots;
                            target->second[slot_index]->setBounds(slot_left, box_bounds.getY(),
                                                            slot_right - slot_left, box_bounds.getHeight());
                            target->second[slot_index]->setVisible(true);
                        }
                    }

                    auto open_gl = modulation_box_open_gl_.find(slider);
                    if (open_gl != modulation_box_open_gl_.end()) {
                        for (int slot_index = 0; slot_index < SynthSlider::kNumModulationSlots; ++slot_index) {
                            const int slot_left = box_bounds.getX() + (slot_index * box_bounds.getWidth())
                                                        / SynthSlider::kNumModulationSlots;
                            const int slot_right = box_bounds.getX() + ((slot_index + 1) * box_bounds.getWidth())
                                                        / SynthSlider::kNumModulationSlots;
                            const juce::Rectangle<int> slot_bounds(
                                slot_left, box_bounds.getY(), slot_right - slot_left, box_bounds.getHeight());

                            auto& visuals = open_gl->second[slot_index];
	                            if (visuals.body) visuals.body->setBounds(slot_bounds);
	                            if (visuals.border) visuals.border->setBounds(slot_bounds);
	                            if (visuals.label) visuals.label->setBounds(slot_bounds.reduced(2, 1));

	                            const auto aux_bounds = getAuxSlotBounds(slot_bounds.toFloat()).toNearestInt();
	                            if (visuals.aux_body) visuals.aux_body->setBounds(aux_bounds);
	                            if (visuals.aux_border) visuals.aux_border->setBounds(aux_bounds);
	                            if (visuals.aux_label) visuals.aux_label->setBounds(aux_bounds.reduced(1, 0));

	                            const int meter_height = std::max(2, static_cast<int>(slot_bounds.getHeight() * 0.12f));
                            if (visuals.amount) {
                                visuals.amount->setBounds(slot_bounds.getX() + 1,
                                                          slot_bounds.getBottom() - meter_height - 1,
                                                          std::max(0, slot_bounds.getWidth() - 2),
                                                          meter_height);
                            }
                        }
                    }
                }
                x += component_width + widget_margin;
            }
        }

        updateSliderLabels();
        syncModulationSlotOpenGl();
        repaintBackground();
    }

    void ParametersView::syncModulationSlotOpenGl() {
        const auto empty_border_color = juce::Colour::fromRGB(54, 78, 79);

        for (auto& [slider, slots] : modulation_box_targets_) {
            auto visuals_iter = modulation_box_open_gl_.find(slider);
            if (visuals_iter == modulation_box_open_gl_.end())
                continue;

            for (int slot_index = 0; slot_index < SynthSlider::kNumModulationSlots; ++slot_index) {
                auto* slot = slots[slot_index].get();
                auto& visuals = visuals_iter->second[slot_index];
                if (slot == nullptr)
                    continue;

	                const bool occupied = slot->isOccupied();
	                const auto source_color = occupied ? slot->getSourceColor() : empty_border_color;
	                const bool has_aux = slot->hasAuxSource();
	                const auto aux_color = has_aux ? slot->getAuxSourceColor() : empty_border_color;
	                const float amount = juce::jlimit(0.0f, 1.0f, slot->getModulationAmount());
	                const auto slot_bounds = slot->getBounds();

                if (visuals.body) {
                    visuals.body->setColor(source_color.withAlpha(0.28f));
                    visuals.body->setVisible(occupied);
                }

                if (visuals.amount) {
                    visuals.amount->setColor(source_color.withAlpha(0.45f));
                    visuals.amount->setVisible(occupied && amount > 0.0f);
                    auto amount_bounds = visuals.amount->getBounds();
                    amount_bounds.setWidth(std::max(0, static_cast<int>(
                        std::round((slot_bounds.getWidth() - 2) * amount))));
                    visuals.amount->setBounds(amount_bounds);
                }

                if (visuals.border) {
                    visuals.border->setColor(source_color);
                    visuals.border->setVisible(true);
                }

	                if (visuals.label) {
	                    visuals.label->setText(occupied ? slot->getSourceLabel() : "");
	                    visuals.label->setTextSize(std::max(9.0f, slot_bounds.getHeight() * 0.45f));
	                    visuals.label->setColor(source_color);
	                    visuals.label->setVisible(occupied);
	                }

	                if (visuals.aux_body) {
	                    visuals.aux_body->setColor(aux_color.withAlpha(0.32f));
	                    visuals.aux_body->setVisible(occupied && has_aux);
	                }

	                if (visuals.aux_border) {
	                    visuals.aux_border->setColor(aux_color);
	                    visuals.aux_border->setVisible(occupied && has_aux);
	                }

	                if (visuals.aux_label) {
	                    visuals.aux_label->setText(has_aux ? slot->getAuxSourceLabel() : "");
	                    visuals.aux_label->setTextSize(std::max(7.0f, slot_bounds.getHeight() * 0.28f));
	                    visuals.aux_label->setColor(aux_color);
	                    visuals.aux_label->setVisible(occupied && has_aux);
	                }
	            }
	        }
	    }

    void ParametersView::init_() {
//        pimpl->view.setRootItem(&pimpl->groupItem);
    }
    juce::Component* ParametersView::getComponentForParameter (const juce::RangedAudioParameter& param) {
//        return pimpl->getComponentForParameter (param);
        return nullptr;
    }

// ============================================================
// FxModuleTemplateView
// ============================================================
FxModuleTemplateView::FxModuleTemplateView(chowdsp::PluginState& pluginState,
                                           chowdsp::ParamHolder& params,
                                           juce::String name)
    : SynthSection(name)
{
    setComponentID(name);
    setInterceptsMouseClicks(false, true);

    params.doForAllParameterContainers(
        [this, &pluginState](auto& paramVec) {
            for (auto& param : paramVec) {
                if ((int)comps.size() < kMaxEffectSlots)
                    comps.push_back(parameters_view_detail::createParameterComp(pluginState, param, *this));
            }
        },
        [](auto&) {});

    for (int i = (int)comps.size(); i < kMaxEffectSlots; ++i) {
        auto ph = std::make_unique<NoPopupSynthSlider>("Param " + juce::String(i + 1));
        ph->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        ph->setScrollWheelEnabled(false);
        ph->setEnabled(false);
        addSlider(ph.get(), true);
        ph->parentHierarchyChanged();
        ph->setActive(false);
        ph->setAlpha(0.3f, true);
        placeholders_.push_back(std::move(ph));
    }

    mix_knob_ = std::make_unique<NoPopupSynthSlider>("Mix");
    mix_knob_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mix_knob_->setScrollWheelEnabled(false);
    mix_knob_->setKnobSizeScale(1.0f);
    addSlider(mix_knob_.get(), true);
    mix_knob_->parentHierarchyChanged();

    postgain_knob_ = std::make_unique<NoPopupSynthSlider>("PostGain");
    postgain_knob_->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    postgain_knob_->setScrollWheelEnabled(false);
    postgain_knob_->setKnobSizeScale(1.0f);
    addSlider(postgain_knob_.get(), true);
    postgain_knob_->parentHierarchyChanged();

    setLookAndFeel(DefaultLookAndFeel::instance());
    setOpaque(true);
    ensureLabels();
}

FxModuleTemplateView::~FxModuleTemplateView() = default;

// FX-local layout constants. These shrink the FX knob/tick-arc footprint and tune
// vertical spacing for the narrow FX panel ONLY. They do not touch global skin
// values or shared SynthSlider rendering constants.
namespace {
    constexpr float kFxKnobScale     = 0.7225f; // 0.85 * 0.85 — FX knob/tick-arc footprint
    constexpr int   kFxLabelHeight   = 18;     // tall enough to avoid clipping descenders
    constexpr int   kFxLabelToArcGap = 2;      // label bottom -> top of tick arc
    constexpr int   kFxArcToModGap   = 4;      // bottom of tick arc -> mod box top
    constexpr int   kFxModBoxHeight  = 16;
}

void FxModuleTemplateView::ensureLabels() {
    for (auto* slider : all_sliders_v) {
        if (slider_labels_.count(slider) != 0)
            continue;

        auto label = std::make_shared<PlainTextComponent>(slider->getName() + "_label", slider->getName());
        label->setFontType(PlainTextComponent::kRegular);
        label->setJustification(juce::Justification::centred);
        auto* gl_slider = dynamic_cast<OpenGlSlider*>(slider);
        bool active = gl_slider == nullptr || gl_slider->isActive();
        label->setColor(active ? ShaderColors::kEffectTextColor
                               : ShaderColors::kEffectTextColor.withAlpha(0.3f));
        label->setInterceptsMouseClicks(false, false);
        addOpenGlComponent(label);
        slider_labels_[slider] = label;
    }
}

void FxModuleTemplateView::updateLabels() {
    for (auto* slider : all_sliders_v) {
        auto it = slider_labels_.find(slider);
        if (it == slider_labels_.end())
            continue;

        const auto b = slider->getBounds();
        it->second->setBounds(b.getX(), b.getY() - kFxLabelHeight - kFxLabelToArcGap,
                              b.getWidth(), kFxLabelHeight);
        it->second->setText(slider->getName());
        it->second->setTextSize(getLabelFont().getHeight());
        it->second->setColor(ShaderColors::kEffectTextColor);
        it->second->setVisible(slider->isEnabled());
    }
}

void FxModuleTemplateView::resized() {
    const int w = getWidth();
    const int H = getHeight();

    // Shrink the FX knob/tick-arc footprint locally via each slider's per-instance
    // knob_size_scale_ (used by both the raster and GL rotary paths). This does NOT
    // change global skin values or shared SynthSlider constants, so other sections
    // are unaffected.
    for (auto* slider : all_sliders_v)
        if (auto* ss = dynamic_cast<SynthSlider*>(slider))
            ss->setKnobSizeScale(kFxKnobScale);

    // Rendered knob box, matched to the 85%-scaled arc (scale applies to arc size and
    // thickness together) so the graphic fits its box exactly.
    const int knobPx = std::max(1, (int) std::ceil(
        kFxKnobScale * 2.0f * (findValue(Skin::kKnobArcSize)
                               + findValue(Skin::kKnobArcThickness))));

    // Divide the panel into kRows equal vertical bands. Within each band: label at the
    // top, a full-height mod box anchored at the band's bottom (so its bottom is always
    // <= H), and the knob/arc centered in the remaining space. Gaps flex with H; the
    // knob keeps its FX-local kFxKnobScale size and the mod box keeps full height, so
    // layout can never push a mod box past the panel bottom.
    constexpr int kRows = 5;
    const int rowSlotH = H / kRows;

    auto placeKnob = [&](juce::Component* comp, int centreX, int slotW, int row, int modIdx) {
        if (comp == nullptr)
            return;
        const int slotTop    = row * rowSlotH;
        const int slotBottom = slotTop + rowSlotH;            // <= kRows*rowSlotH <= H
        const int modBoxTop  = slotBottom - kFxModBoxHeight;  // full height, bottom = slotBottom
        // Center the knob/arc in the space between the label and the mod box.
        const int midTop    = slotTop + kFxLabelHeight + kFxLabelToArcGap;
        const int midBottom = modBoxTop - kFxArcToModGap;
        const int arcTop    = midTop + std::max(0, ((midBottom - midTop) - knobPx) / 2);
        comp->setBounds(centreX - knobPx / 2, arcTop, knobPx, knobPx);

        // Mod box: full height, anchored at the band bottom, centered in the slot.
        // Clamp the width to the slot interior (like the oscillator ParametersView boxes)
        // so it can never be wider than its slot / escape the panel.
        constexpr int kFxModBoxMargin = 2;
        const int desired_bw = slotW * 23 / 20;
        const int bw = std::max(0, std::min(desired_bw, slotW - 2 * kFxModBoxMargin));
        mod_boxes_[modIdx] = { centreX - bw / 2, modBoxTop, bw, kFxModBoxHeight };
    };

    auto slotComp = [&](int slotIdx) -> juce::Component* {
        if (slotIdx < (int)comps.size())
            return comps[slotIdx].get();
        const int phIdx = slotIdx - (int)comps.size();
        if (phIdx < (int)placeholders_.size())
            return placeholders_[phIdx].get();
        return nullptr;
    };

    // Row 0: slot 0 (single)
    { int kw = w * 3 / 10; placeKnob(slotComp(0), w / 2, kw, 0, 0); }

    // Row 1: slots 1 & 2 (two columns)
    { int colW = w / 2; int kw = colW / 2;
      placeKnob(slotComp(1), colW / 2,        kw, 1, 1);
      placeKnob(slotComp(2), colW + colW / 2, kw, 1, 2); }

    // Row 2: slots 3 & 4 (two columns)
    { int colW = w / 2; int kw = colW / 2;
      placeKnob(slotComp(3), colW / 2,        kw, 2, 3);
      placeKnob(slotComp(4), colW + colW / 2, kw, 2, 4); }

    // Row 3: Mix
    { int kw = w * 27 / 100; placeKnob(mix_knob_.get(), w / 2, kw, 3, 5); }

    // Row 4: PostGain
    { int kw = w * 27 / 100; placeKnob(postgain_knob_.get(), w / 2, kw, 4, 6); }

    for (auto* slider : all_sliders_v)
        if (auto* synth_slider = dynamic_cast<SynthSlider*>(slider))
            synth_slider->redoImage();
    updateLabels();

    SynthSection::resized();

    // NOTE: Do NOT call repaintBackground() here.
    //
    // repaintBackground() walks up to FullInterface and stamps this view's
    // paintBackground() (all 7 modulation boxes) into the *global* window
    // background image. Unlike the working non-scrolled sections, an FX view
    // lives inside the EffectModuleSection scroll viewport: its absolute bounds
    // extend past the visible viewport and shift on scroll/reflow, and the
    // global image is neither viewport-clipped nor cleared at the view's old
    // position. That is what made the rectangular mod boxes escape the FX panel
    // while scrolling and linger after a module was removed.
    //
    // The FX panel's visible background comes solely from the scroll-aware,
    // viewport-scissored image baked in EffectModuleSection::redoBackgroundImage(),
    // which the owning EffectModuleSection::resized() always rebakes after it
    // lays out the FX views. So the scroll background stays correct without the
    // harmful global stamp. This is FX-local: other sections still repaint
    // normally because they are not inside a scrolling viewport.
}

static void drawModulationBox(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour col) {
    auto r = bounds.toFloat();
    g.setColour(col);
    g.drawRect(r, 1.0f);
    float x1 = r.getX() + r.getWidth() / 3.0f;
    float x2 = r.getX() + r.getWidth() * 2.0f / 3.0f;
    g.drawLine(x1, r.getY(), x1, r.getBottom(), 1.0f);
    g.drawLine(x2, r.getY(), x2, r.getBottom(), 1.0f);
}

void FxModuleTemplateView::paintBackground(juce::Graphics& g) {
    SynthSection::paintContainer(g);
    paintBorder(g);
    paintKnobShadows(g);
    paintChildrenBackgrounds(g);

    // Dim placeholder knob face area with a dark overlay
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    for (auto& ph : placeholders_)
        g.fillRect(ph->getBounds());

    // Slider labels render as OpenGL PlainTextComponents (see ensureLabels/updateLabels).

    // Modulation boxes — muted color for placeholder slots (indices comps.size()..kMaxEffectSlots-1)
    // boxes that go under knobs to indicate routing
    const juce::Colour activeBoxCol = juce::Colour::fromRGB(54, 78, 79);
    const juce::Colour inactiveBoxCol = juce::Colour::fromRGB(54, 78, 79).withAlpha(0.5f);
    for (int i = 0; i < (int)mod_boxes_.size(); ++i) {
        bool isPlaceholder = (i >= (int)comps.size() && i < kMaxEffectSlots);
        drawModulationBox(g, mod_boxes_[i], isPlaceholder ? inactiveBoxCol : activeBoxCol);
    }
}

}//naemspace bitlkavier
