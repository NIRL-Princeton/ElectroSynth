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
    :
    SynthSection(name)
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

    void ParametersView::paint(juce::Graphics &g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void ParametersView::resized() {
        //DBG("--------" + getName() + "View -------------");
        //DBG("bounds x:" + juce::String(getLocalBounds().getX()) + " y:" + juce::String(getLocalBounds().getY()) + " width: " + juce::String(getLocalBounds().getWidth()) + " height: " + juce::String(getLocalBounds().getHeight()));
        //pimpl->groupItem.setBounds(getLocalBounds());

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
            const int height = row_area.getHeight() - widget_margin;

            for (int i = first; i < last; ++i) {
                const int left = std::round(x);
                const int right = std::round(x + component_width);
                if (auto* slider = dynamic_cast<SynthSlider*> (all_sliders_v[i])) {
                    slider->setBounds(left, row_area.getY(), right - left, height);
                    slider->redoImage();
                }
                x += component_width + widget_margin;
            }
        }
//        SynthSection::resized();
//        juce::Grid g;
//
//        placeKnobsInArea(getLocalBounds(),
//                         getAllSlidersVec());
//        for(auto slider : getAllSlidersVec())
//        {
//            DBG("setslider" + slider->getName());
//        }
//        for (auto subsection: sub_sections_)
//        {
//            g.items.add(juce::GridItem(subsection));
//        }
//        g.performLayout(getLocalBounds());
    }


    void ParametersView::init_()
    {
//        pimpl->view.setRootItem(&pimpl->groupItem);
    }
    juce::Component* ParametersView::getComponentForParameter (const juce::RangedAudioParameter& param)
    {
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
}

FxModuleTemplateView::~FxModuleTemplateView() = default;

void FxModuleTemplateView::resized() {
    const int labelH = 12;
    const int labelGap = 14;
    const int vPad = labelH + labelGap + 6;
    const int rowH = (getHeight() - vPad - 4) / 5;
    const int knobH = rowH / 2;
    const int spacedRowH = knobH * 54 / 25;
    const int modH = 16;   // 14 * 1.15
    const int modGap = 9;
    const int w = getWidth();
    int y = vPad - 3;

    auto setSlotBounds = [&](int slotIdx, juce::Rectangle<int> b, int modIdx) {
        if (slotIdx < (int)comps.size())
            comps[slotIdx]->setBounds(b);
        else if (slotIdx - (int)comps.size() < (int)placeholders_.size())
            placeholders_[slotIdx - (int)comps.size()]->setBounds(b);
        int bw = b.getWidth() * 23 / 20;
        int bx = b.getX() - (bw - b.getWidth()) / 2;
        mod_boxes_[modIdx] = { bx, b.getBottom() + modGap, bw, modH };
    };

    // Row 1: slot 0
    { int kw = w * 3 / 10; setSlotBounds(0, { (w - kw) / 2, y, kw, knobH }, 0); y += spacedRowH; }

    // Row 2: slots 1 & 2
    { int colW = w / 2; int kw = colW / 2;
      setSlotBounds(1, { (colW - kw) / 2,       y, kw, knobH }, 1);
      setSlotBounds(2, { colW + (colW - kw) / 2, y, kw, knobH }, 2);
      y += spacedRowH; }

    // Row 3: slots 3 & 4
    { int colW = w / 2; int kw = colW / 2;
      setSlotBounds(3, { (colW - kw) / 2,       y, kw, knobH }, 3);
      setSlotBounds(4, { colW + (colW - kw) / 2, y, kw, knobH }, 4);
      y += spacedRowH; }

    // Row 4: Mix
    { int kw = w * 27 / 100; juce::Rectangle<int> b { (w - kw) / 2, y, kw, knobH };
      mix_knob_->setBounds(b);
      { int bw = b.getWidth() * 23 / 20; mod_boxes_[5] = { b.getX() - (bw - b.getWidth()) / 2, b.getBottom() + modGap, bw, modH }; }
      y += spacedRowH; }

    // Row 5: PostGain
    { int kw = w * 27 / 100; juce::Rectangle<int> b { (w - kw) / 2, y, kw, knobH };
      postgain_knob_->setBounds(b);
      { int bw = b.getWidth() * 23 / 20; mod_boxes_[6] = { b.getX() - (bw - b.getWidth()) / 2, b.getBottom() + modGap, bw, modH }; } }

    SynthSection::resized();
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
    // paintHeadingText(g); // disabled: processor name shown by parent ModuleSection
    paintBorder(g);
    paintKnobShadows(g);
    paintChildrenBackgrounds(g);

    // Dim placeholder knob face area with a dark overlay
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    for (auto& ph : placeholders_)
        g.fillRect(ph->getBounds());

    // Labels — skip disabled (placeholder) slots
    const int labelH = 12;
    g.setFont(Fonts::instance()->proportional_regular().withPointHeight(9.0f));
    g.setColour(findColour(Skin::kBodyText, true));
    for (auto slider : all_sliders_v) {
        if (!slider->isEnabled()) continue;
        auto b = slider->getBounds();
        int labelY = b.getY() - labelH - 11;
        g.drawText(slider->getName(), b.getX(), labelY,
                   b.getWidth(), labelH, juce::Justification::centred, false);
    }

    // Modulation boxes — muted color for placeholder slots (indices comps.size()..kMaxEffectSlots-1)
    const juce::Colour activeBoxCol   = juce::Colour::fromRGB(54, 78, 79);
    const juce::Colour inactiveBoxCol = juce::Colour::fromRGB(54, 78, 79).withAlpha(0.5f);
    for (int i = 0; i < (int)mod_boxes_.size(); ++i) {
        bool isPlaceholder = (i >= (int)comps.size() && i < kMaxEffectSlots);
        drawModulationBox(g, mod_boxes_[i], isPlaceholder ? inactiveBoxCol : activeBoxCol);
    }
}

}//naemspace bitlkavier
