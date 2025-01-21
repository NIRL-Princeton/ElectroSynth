#include "ParametersView.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "open_gl_background.h"
#include "open_gl_combobox.h"
namespace electrosynth {
    namespace parameters_view_detail {

        //==============================================================================
        class BooleanParameterComponent : public juce::Component {
        public:
            BooleanParameterComponent(chowdsp::BoolParameter &param, chowdsp::ParameterListeners& listeners,SynthSection &parent)
                    : button(param.name), attachment(param, listeners, button, nullptr) {
                button.setComponentID(param.paramID);
                setLookAndFeel(DefaultLookAndFeel::instance());
                parent.addButton(&button);

                //parent.addGlComponent (button.getGlComponent());
                //addAndMakeVisible(button);
            }

            void resized() override {
                // auto area = getLocalBounds();
                // area.removeFromLeft(8);
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
            ChoiceParameterComponent(chowdsp::ChoiceParameter &param, chowdsp::ParameterListeners& listeners,SynthSection &parent)
                    : attachment(param, listeners, box, nullptr) {
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
            SliderParameterComponent(chowdsp::FloatParameter &param, chowdsp::ParameterListeners& listeners, SynthSection &parent)
                    : slider(param.name), attachment(param, listeners, slider, nullptr) {
                slider.setComponentID(param.paramID);
                setLookAndFeel(DefaultLookAndFeel::instance());
                slider.setScrollWheelEnabled(false);
                addAndMakeVisible(slider);
                parent.addSlider(&slider, false);
                slider.parentHierarchyChanged();
                slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                _ASSERT(slider.getSectionParent() != nullptr);
                DBG("create slider for " + param.paramID + "with parent " + parent.getName());
             }

             ~SliderParameterComponent()
             {
//                auto parent = findParentComponentOfClass<SynthGuiInterface>();
//
//                parent->getOpenGlWrapper()

             }

            void resized() override {
                auto area = getBoundsInParent();
                slider.setBounds(area);
                slider.redoImage();
            }

        private:
            SynthSlider slider;
            //juce::Slider slider { juce::Slider::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxRight };
            chowdsp::SliderAttachment attachment;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SliderParameterComponent)
        };
        std::unique_ptr<juce::Component> createParameterComp(chowdsp::ParameterListeners& listeners, juce::RangedAudioParameter &parameter, SynthSection& parent) {
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
            : ParametersView (pluginState.getParameterListeners(), params, name) {

    }

    ParametersView::ParametersView(chowdsp::ParameterListeners& paramListeners, chowdsp::ParamHolder& params, String name)
            :  SynthSection(name)
    {
        setComponentID(name);
              //pimpl(std::make_unique<Pimpl>(params, paramListeners, *this)){
//        auto *viewport = pimpl->view.getViewport();
        params.doForAllParameterContainers(
                [this, &paramListeners](auto &paramVec) {
                    for (auto &param: paramVec)
                    {
                        comps.push_back(parameters_view_detail::createParameterComp(paramListeners, param,*this));

                    }
                },
                [this, &paramListeners](auto &paramHolder) {
                    DBG("add group item");
//                    addSubSection(std::make_unique<ParameterGroupItem>(paramHolder,listeners, *this).release());
                });
        setLookAndFeel(DefaultLookAndFeel::instance());
        setOpaque(true);
//        addAndMakeVisible(pimpl->view);
//        viewport->setScrollBarsShown (true, false);
//        setSize(viewport->getViewedComponent()->getWidth() + viewport->getVerticalScrollBar().getWidth(),
//                juce::jlimit(125, 700, viewport->getViewedComponent()->getHeight()));
    }

    ParametersView::~ParametersView() = default;

    void ParametersView::paint(juce::Graphics &g) {
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    }

    void ParametersView::resized() {
        DBG("--------" + getName() + "View -------------");
        DBG("bounds x:" + juce::String(getLocalBounds().getX()) + " y:" + juce::String(getLocalBounds().getY()) + " width: " + juce::String(getLocalBounds().getWidth()) + " height: " + juce::String(getLocalBounds().getHeight()));
        //pimpl->groupItem.setBounds(getLocalBounds());
        placeKnobsInArea(getLocalBounds(), comps);
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
    }
}//naemspace bitlkavier