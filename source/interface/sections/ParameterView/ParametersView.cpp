#include "ParametersView.h"
#include "FxModuleTemplateView.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "open_gl_background.h"
#include "open_gl_combobox.h"
#include <cmath>

// ParametersView.cpp is the generic parameter-to-controller builder. Given a processor’s parameter list, it creates the right UI component
// for each parameter:
// BoolParameter -> toggle button
// ChoiceParameter -> combo box
// FloatParameter -> SynthSlider
// Each class is a small wrapper object that makes the UI component, attaches it to the parameter, and registers it with the SynthSection parent

namespace electrosynth {

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
                setComponentID(param.paramID);
                addAndMakeVisible(box);
                parent.addChildComponent (box);
                parent.addOpenGlComponent (box.getImageComponent());
            }

            void setControlBounds(juce::Rectangle<int> bounds) {
                setBounds(bounds);
                box.setBounds(bounds);
            }

            void configureFilterSelector(std::function<void(int)> selection_changed) {
                box.setColour(juce::ComboBox::outlineColourId, juce::Colours::white);
                box.getImageComponent()->setRenderScale(2.0f);
                box.onChange = [this, callback = std::move(selection_changed)] {
                    callback(box.getSelectedItemIndex());
                };
                box.redoImage();
            }

            int getSelectedItemIndex() const { return box.getSelectedItemIndex(); }

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
                        auto component = parameters_view_detail::createParameterComp(pluginState, param, *this);
                        if (param->paramID == "filterType") {
                            if (auto* choice = dynamic_cast<parameters_view_detail::ChoiceParameterComponent*>(component.get())) {
                                filter_type_layout_ = [choice](juce::Rectangle<int> bounds) {
                                    choice->setControlBounds(bounds);
                                };
                                filter_type_index_ = choice->getSelectedItemIndex();
                                choice->configureFilterSelector([this](int index) {
                                    filter_type_index_ = index;
                                    updateSliderLabels();
                                });
                            }
                        }
                        comps.push_back(std::move(component));
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

    int ParametersView::getKnobsPerRow() const {
        return getName().startsWithIgnoreCase("string") ? kStringKnobsPerRow : kDefaultKnobsPerRow;
    }

    int ParametersView::getKnobRowCount() const {
        if (all_sliders_v.empty())
            return 1;

        return static_cast<int> (std::ceil (all_sliders_v.size() / static_cast<float> (getKnobsPerRow())));
    }

    int ParametersView::getPreferredHeight() const {
        constexpr int kFilterTypeRowHeight = 28;
        return getKnobRowCount() * kModuleHeightPerKnobRow
               + (filter_type_layout_ ? kFilterTypeRowHeight : 0);
    }

    juce::Colour ParametersView::getSliderLabelColor() const {
        return findColour(Skin::kBodyText, true);
    }

    juce::String ParametersView::getSliderLabel(const juce::Component& slider) const {
        if (!filter_type_layout_)
            return slider.getName();

        const bool peak = filter_type_index_ == 4;
        const bool shelf = filter_type_index_ == 5 || filter_type_index_ == 6;
        if (slider.getName().equalsIgnoreCase("Cutoff"))
            return peak || shelf ? "Frequency" : "Cutoff";
        if (slider.getName().equalsIgnoreCase("Q"))
            return peak ? "Bandwidth" : "Resonance";
        if (slider.getName().equalsIgnoreCase("Amp"))
            return peak || shelf ? "Gain" : "Output";
        return slider.getName();
    }

    void ParametersView::paint(juce::Graphics &g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void ParametersView::paintBackground(juce::Graphics& g) {
        SynthSection::paintContainer(g);
        paintBorder(g);
        paintKnobShadows(g);
        paintChildrenBackgrounds(g);
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

            if (auto* synth_slider = dynamic_cast<SynthSlider*>(slider))
                ensureModulationSlots(*synth_slider);
        }
    }

    void ParametersView::ensureModulationSlots(SynthSlider& slider) {
        if (modulation_slot_strips_.contains(&slider))
            return;

        auto strip = std::make_unique<ModulationSlots>(slider);
        addSubSection(strip.get());
        modulation_slot_strips_[&slider] = std::move(strip);
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
            it->second->setText(getSliderLabel(*slider));
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
        constexpr int kFilterTypeRowHeight = 28;
        constexpr int kFilterTypeHeight = 20;
        const int content_top = filter_type_layout_ ? kFilterTypeRowHeight : 0;
        if (filter_type_layout_) {
            const int combo_width = juce::jlimit(120, 220, getWidth() / 3);
            filter_type_layout_(juce::Rectangle<int>((getWidth() - combo_width) / 2,
                                                      (kFilterTypeRowHeight - kFilterTypeHeight) / 2,
                                                      combo_width, kFilterTypeHeight));
        }
        const int row_height = std::max(1, (getHeight() - content_top) / num_rows);

        for (int row = 0; row < num_rows; ++row) {
            const int first = row * knobs_per_row;
            const int last = std::min<int>(first + knobs_per_row, all_sliders_v.size());
            const int count = last - first;
            if (count <= 0)
                continue;

            juce::Rectangle<int> row_area(0, content_top + row * row_height,
                                          getWidth(), row_height);
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
                    if (auto strip = modulation_slot_strips_.find(slider);
                        strip != modulation_slot_strips_.end())
                        strip->second->setBounds(box_bounds);
                }
                x += component_width + widget_margin;
            }
        }

        updateSliderLabels();
        repaintBackground();
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
                if (auto* choice = dynamic_cast<chowdsp::ChoiceParameter*>(param.get());
                    choice != nullptr && param->paramID == "filterType") {
                    filter_type_combo_ = std::make_unique<OpenGLComboBox>();
                    filter_type_combo_->setColour(juce::ComboBox::outlineColourId,
                                                  juce::Colours::white);
                    filter_type_combo_->getImageComponent()->setRenderScale(2.0f);
                    addAndMakeVisible(filter_type_combo_.get());
                    addOpenGlComponent(filter_type_combo_->getImageComponent());
                    filter_type_attachment_ = std::make_unique<chowdsp::ComboBoxAttachment>(
                        *choice, pluginState, *filter_type_combo_);
                    filter_type_index_ = filter_type_combo_->getSelectedItemIndex();
                    filter_type_combo_->onChange = [this] {
                        filter_type_index_ = filter_type_combo_->getSelectedItemIndex();
                        updateLabels();
                    };
                    continue;
                }

                if ((int)comps.size() < kMaxEffectSlots)
                    comps.push_back(parameters_view_detail::createParameterComp(pluginState, param, *this));
            }
        },
        [](auto&) {});

    // Mix / PostGain are intended visible FX controls. NOTE: they are currently
    // UI-only (no chowdsp parameter attachment) and are NOT wired to DSP yet.
    // They use a dedicated utility layout below the real rotary parameters.
    mix_knob_ = std::make_unique<SynthSlider>("Mix");
    mix_knob_->setSliderStyle(juce::Slider::LinearBar);
    mix_knob_->setScrollWheelEnabled(false);
    mix_knob_->setKnobSizeScale(1.0f);
    addSlider(mix_knob_.get(), true);
    mix_knob_->parentHierarchyChanged();

    postgain_knob_ = std::make_unique<SynthSlider>("PostGain");
    postgain_knob_->setSliderStyle(juce::Slider::LinearBar);
    postgain_knob_->setScrollWheelEnabled(false);
    postgain_knob_->setKnobSizeScale(1.0f);
    addSlider(postgain_knob_.get(), true);
    postgain_knob_->parentHierarchyChanged();

    setLookAndFeel(DefaultLookAndFeel::instance());
    setOpaque(false);
    ensureLabels();
}

FxModuleTemplateView::~FxModuleTemplateView() = default;

// FX-local layout constants. FX rotaries intentionally use the same shared default
// scale as oscillator controls; spacing remains local to the narrow FX panel.
namespace {
    constexpr float kFxKnobScale       = OpenGlSlider::kDefaultKnobSizeScale;
    constexpr int   kFxLabelHeight     = 18;      // tall enough to avoid clipping descenders
    constexpr int   kFxLabelToArcGap   = 2;       // label bottom -> top of tick arc
    constexpr int   kFxRowTopPad       = 10;      // padding above the first row
    constexpr int   kFxRowBottomPad    = 2;       // padding below the last row
    constexpr int   kFxRowGap          = 10;      // breathing room between stacked rows
    constexpr int   kFxMinKnobCellWidth = 76;     // min horizontal cell per knob (drives knobs-per-row)
    constexpr int   kFxSideInset       = 1;       // side inset = border thickness (paintBorder draws 1px)
    constexpr int   kFxModulationBoxGap = 4;
    constexpr int   kFxModulationBoxWidth = 120;
    constexpr int   kFxUtilityTopGap = 20;
    constexpr int   kFxUtilityLabelWidth = 60;
    constexpr int   kFxUtilitySideInset = 20;
    constexpr int   kFxUtilitySliderHeight = 20;
    constexpr int   kFxUtilityRowGap = 10;
    // ModuleSection draws the audio-port arrows over its bottom content area.
    constexpr int   kFxAudioPortReserve = 30;
    // Filter type dropdown: doubled top gap hosts the control; height matches the lane
    // header's routing dropdown (kRoutingControlHeight in EffectsModuleSection).
    constexpr int   kFxTypeComboHeight = 14;
}

juce::Colour FxModuleTemplateView::getLabelColor(const juce::Component* control) const {
    return ShaderColors::kEffectTextColor;
}

juce::String FxModuleTemplateView::getControlLabel(const juce::Component& control) const {
    if (filter_type_combo_ == nullptr)
        return control.getName();

    const bool peak = filter_type_index_ == 4;
    const bool shelf = filter_type_index_ == 5 || filter_type_index_ == 6;
    if (control.getName().equalsIgnoreCase("Cutoff"))
        return peak || shelf ? "Frequency" : "Cutoff";
    if (control.getName().equalsIgnoreCase("Q"))
        return peak ? "Bandwidth" : "Resonance";
    if (control.getName().equalsIgnoreCase("Amp"))
        return peak || shelf ? "Gain" : "Output";
    return control.getName();
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
        const auto label_color = getLabelColor(slider);
        label->setColor(active ? label_color : label_color.withAlpha(0.3f));
        label->setInterceptsMouseClicks(false, false);
        addOpenGlComponent(label);
        slider_labels_[slider] = label;

        if (auto* synth_slider = dynamic_cast<SynthSlider*>(slider);
            synth_slider != nullptr && synth_slider != mix_knob_.get()
            && synth_slider != postgain_knob_.get())
            ensureModulationSlots(*synth_slider);
    }
}

void FxModuleTemplateView::ensureModulationSlots(SynthSlider& slider) {
    if (modulation_slot_strips_.contains(&slider))
        return;

    auto strip = std::make_unique<ModulationSlots>(slider);
    addSubSection(strip.get());
    modulation_slot_strips_[&slider] = std::move(strip);
}

void FxModuleTemplateView::updateLabels() {
    for (auto* slider : all_sliders_v) {
        auto it = slider_labels_.find(slider);
        if (it == slider_labels_.end())
            continue;

        const auto b = slider->getBounds();
        if (slider != mix_knob_.get() && slider != postgain_knob_.get())
            it->second->setBounds(b.getX(), b.getY() - kFxLabelHeight - kFxLabelToArcGap,
                                  b.getWidth(), kFxLabelHeight);
        it->second->setText(getControlLabel(*slider));
        it->second->setTextSize(getLabelFont().getHeight());
        it->second->setColor(getLabelColor(slider));
        it->second->setVisible(slider->isEnabled());
    }
}

// FX-only preferred height, mirroring resized()'s dynamic layout: derive the row count
// the same way (visible controls, knobs-per-row from lane width) and size for exactly
// that many rows plus padding. No fixed module height.
int FxModuleTemplateView::getPreferredHeight() const {
    // Filter modules double the title-to-first-label gap to host the type dropdown.
    const int top_pad = filter_type_combo_ != nullptr ? 2 * kFxRowTopPad : kFxRowTopPad;

    const int n = static_cast<int>(comps.size());

    const int knobPx = std::max(1, (int) std::ceil(
        kFxKnobScale * 2.0f * (findValue(Skin::kKnobArcSize)
                               + findValue(Skin::kKnobArcThickness))));

    const int perRow = std::max(1, std::min(getWidth() / kFxMinKnobCellWidth,
                                            std::max(1, n)));
    const int numRows = n > 0 ? (n + perRow - 1) / perRow : 0;
    const int rowContentH = kFxLabelHeight + kFxLabelToArcGap + knobPx
                            + kFxModulationBoxGap + ModulationSlots::kHeight;
    const int parameter_height = numRows > 0
        ? numRows * rowContentH + (numRows - 1) * kFxRowGap
        : 0;
    const int utility_height = kFxUtilityTopGap + 2 * kFxUtilitySliderHeight
                             + kFxUtilityRowGap + kFxAudioPortReserve;

    return top_pad + parameter_height + kFxRowBottomPad + utility_height;
}

void FxModuleTemplateView::resized() {
    // 1. Lane width.
    const int w = getWidth();

    // 2. Shrink the FX knob/tick-arc footprint locally (raster + GL rotary). Does NOT
    // change global skin values or shared SynthSlider constants.
    for (auto* slider : all_sliders_v)
        if (auto* ss = dynamic_cast<SynthSlider*>(slider);
            ss != nullptr && ss != mix_knob_.get() && ss != postgain_knob_.get())
            ss->setKnobSizeScale(kFxKnobScale);

    // 3. Rendered knob box, matched to the FX-scaled arc.
    const int knobPx = std::max(1, (int) std::ceil(
        kFxKnobScale * 2.0f * (findValue(Skin::kKnobArcSize)
                               + findValue(Skin::kKnobArcThickness))));

    // 4. Real parameter controls remain rotary. Mix and PostGain are laid out
    // separately as utility bars below them.
    std::vector<juce::Component*> controls;
    controls.reserve(comps.size());
    for (auto& c : comps)
        controls.push_back(c.get());

    // 5. Knobs per row = as many as fit the lane width, clamped to [1, n].
    const int n = (int) controls.size();
    // perRow ignores the side inset; the inset only trims the row area used for centering.
    const int perRow = std::max(1, std::min(w / kFxMinKnobCellWidth,
                                            std::max(1, n)));

    // 6. Row sizes. remainder becomes a smaller FIRST row so every later row (incl. the
    // final Mix/PostGain row) is full; single-per-row when only one fits.
    std::vector<int> rows;
    if (n == 0) {
        rows.clear();
    } else if (perRow <= 1) {
        rows.assign(n, 1);
    } else {
        const int leftover = n % perRow;
        if (leftover != 0)
            rows.push_back(leftover);
        for (int placed = leftover; placed < n; placed += perRow)
            rows.push_back(perRow);
    }

    // 7. Position controls row by row, each row centered horizontally within the inset area.
    // Filter modules double the top gap and center the placeholder type dropdown in it,
    // sized/styled like the lane header's routing dropdown.
    const int top_pad = filter_type_combo_ != nullptr ? 2 * kFxRowTopPad : kFxRowTopPad;
    if (filter_type_combo_ != nullptr) {
        const int combo_h = std::min(kFxTypeComboHeight, top_pad);
        const int combo_w = std::max(60, w / 2);
        filter_type_combo_->setBounds((w - combo_w) / 2, (top_pad - combo_h) / 2,
                                      combo_w, combo_h);
    }

    const int rowContentH = kFxLabelHeight + kFxLabelToArcGap + knobPx
                            + kFxModulationBoxGap + ModulationSlots::kHeight;
    const int cellW = (w - 2 * kFxSideInset) / perRow;
    int idx = 0;
    int y = top_pad;
    for (int cnt : rows) {
        const int startX = kFxSideInset + ((w - 2 * kFxSideInset) - cnt * cellW) / 2;
        const int arcTop = y + kFxLabelHeight + kFxLabelToArcGap;
        for (int c = 0; c < cnt && idx < n; ++c, ++idx) {
            const int centreX = startX + c * cellW + cellW / 2;
            controls[idx]->setBounds(centreX - knobPx / 2, arcTop, knobPx, knobPx);
        }
        y += rowContentH + kFxRowGap;
    }

    const int parameter_bottom = rows.empty() ? top_pad : y - kFxRowGap;
    const int utility_y = parameter_bottom + kFxRowBottomPad + kFxUtilityTopGap;
    const int utility_width = std::max(0, w - 2 * kFxUtilitySideInset);

    auto layoutUtilitySlider = [this, utility_width](SynthSlider& slider, int row_y) {
        auto row = juce::Rectangle<int>(kFxUtilitySideInset, row_y, utility_width,
                                        kFxUtilitySliderHeight);
        const auto label_bounds = row.removeFromLeft(
            std::min(kFxUtilityLabelWidth, row.getWidth()));
        slider.setBounds(row);
        slider.redoImage();

        if (auto label = slider_labels_.find(&slider); label != slider_labels_.end())
            label->second->setBounds(label_bounds);
    };

    layoutUtilitySlider(*mix_knob_, utility_y);
    layoutUtilitySlider(*postgain_knob_,
                        utility_y + kFxUtilitySliderHeight + kFxUtilityRowGap);

    // 8. Redo slider images + labels.
    for (auto* slider : all_sliders_v)
        if (auto* synth_slider = dynamic_cast<SynthSlider*>(slider)) {
            synth_slider->redoImage();
            if (auto strip = modulation_slot_strips_.find(synth_slider);
                strip != modulation_slot_strips_.end()) {
                const int box_width = std::max(0, std::min(kFxModulationBoxWidth, cellW - 4));
                strip->second->setBounds(synth_slider->getBounds().getCentreX() - box_width / 2,
                                         synth_slider->getBottom() + kFxModulationBoxGap,
                                         box_width, ModulationSlots::kHeight);
            }
        }
    updateLabels();

    // 9. Base resize.
    SynthSection::resized();

    // 10. Existing note, kept once.
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

void FxModuleTemplateView::paintBackground(juce::Graphics& g) {
    SynthSection::paintContainer(g);
    paintBorder(g);
    paintKnobShadows(g);
    paintChildrenBackgrounds(g);

    // NOTE: this function is not invoked in the FX lane path — ModuleSection::
    // paintBackground() is intentionally empty and never paints its child view, so the
    // combo outline is part of the combo's own high-resolution OpenGL image.

    // FX modulation boxes are hidden for now, but the layout still reserves their space
    // to preserve row spacing (see mod_boxes_ / kFxModBoxHeight in resized()). The old
    // drawModulationBox() rendering pass is intentionally omitted so no boxes are drawn.

    // Slider labels render as OpenGL PlainTextComponents (see ensureLabels/updateLabels).
}

}//naemspace bitlkavier
