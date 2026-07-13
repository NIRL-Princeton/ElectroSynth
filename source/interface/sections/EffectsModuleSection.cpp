//
// Created by Davis Polito on 11/19/24.
//

#include "EffectsModuleSection.h"
#include "ModuleSection.h"
#include "synth_gui_interface.h"
#include "Processors/ProcessorBase.h"
#include "modulation_manager.h"
#include "synth_base.h"
#include "EffectList.h"

namespace {
class FxAddButtonBackground final : public OpenGlImageComponent {
public:
    FxAddButtonBackground() : OpenGlImageComponent("effect_add_button_background") {
        setInterceptsMouseClicks(false, false);
    }

    void resized() override {
        redrawImage(true);
    }

    void paintToImage(juce::Graphics& g) override {
        const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        const auto base = findColour(Skin::kBorder, true);
        g.setColour(findColour(Skin::kBackground, true).withAlpha(0.45f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 1.0f), 5.0f);

        juce::ColourGradient gradient(base.brighter(0.08f), 0.0f, bounds.getY(),
                                      base.darker(0.10f), 0.0f, bounds.getBottom(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, 5.0f);
    }
};
}

EffectModuleSection::EffectModuleSection(ModulationManager *m, EffectList &module_list,const juce::ValueTree &v, juce::UndoManager& um) :
ModulesInterface( module_list), footer_body(new OpenGlQuad(Shaders::kRoundedRectangleFragment)), state(v), undo(um)
{
    scroll_bar_ = std::make_unique<OpenGlScrollBar>();
    addAndMakeVisible(scroll_bar_.get());
    addOpenGlComponent(scroll_bar_->getGlComponent());
    addOpenGlComponent(footer_body);

    setLookAndFeel(DefaultLookAndFeel::instance());
    scroll_bar_->addListener(this);
    viewport_.setScrollBarPosition(true, false); //use this to determine viewport scroll type in effectsviewport
    viewport_.setScrollBarsShown(false, false, true, false);

    addListener(m);
    for (auto obj : list) {
        EffectModuleSection::moduleAdded(obj);
    }
    setSidewaysHeading(false);
    // Lane label derives from the EffectList's lane index (0/1/2) rather than a
    // hardcoded "FX" string, so the three lanes read "Lane 1/2/3".
    setName("Lane " + juce::String(module_list.lane + 1));

    header_body_ = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, "effect_module_header");
    header_body_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(header_body_, true);

    header_title_ = std::make_shared<PlainTextComponent>("effect_module_title", getName());
    header_title_->setFontType(PlainTextComponent::kLight);
    header_title_->setJustification(juce::Justification::centred);
    header_title_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(header_title_);

    // FX routing state is not currently exposed. Keep the lane-header control
    // visibly present but disabled rather than attaching it to sound-chain routing.
    routing_combo_box_ = std::make_unique<OpenGLComboBox>();
    routing_combo_box_->addItem("Master", 1);
    routing_combo_box_->setSelectedId(1, juce::dontSendNotification);
    routing_combo_box_->setInterceptsMouseClicks(false, false);
    routing_combo_box_->setWantsKeyboardFocus(false);
    addAndMakeVisible(routing_combo_box_.get());
    addOpenGlComponent(routing_combo_box_->getImageComponent());

    add_effect_button_background_ = std::make_shared<FxAddButtonBackground>();
    addOpenGlComponent(add_effect_button_background_);

    add_effect_button_ = std::make_unique<OpenGlShapeButton>("Add Effect");
    addAndMakeVisible(add_effect_button_.get());
    addOpenGlComponent(add_effect_button_->getGlComponent());
    add_effect_button_->addListener(this);
    add_effect_button_->addMouseListener(this, false);
    add_effect_button_->setShape(Paths::plus(150));

    // Two coincident overlay passes mirror ModulationModuleSection::paintBackground(),
    // which paints the same 1.0 outline twice. They remain above the scrolling content.
    border_overlay_ = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleBorderFragment, "effect_lane_border");
    border_overlay_second_pass_ = std::make_shared<OpenGlQuad>(
        Shaders::kRoundedRectangleBorderFragment, "effect_lane_border_second_pass");
    border_overlay_->setInterceptsMouseClicks(false, false);
    border_overlay_second_pass_->setInterceptsMouseClicks(false, false);
    border_overlay_->setAlwaysOnTop(true);
    border_overlay_second_pass_->setAlwaysOnTop(true);
    addOpenGlComponent(border_overlay_);
    addOpenGlComponent(border_overlay_second_pass_);

    toggle_button_->setVisible(false);
    setInterceptsMouseClicks(true,true);
}

EffectModuleSection::~EffectModuleSection() {
   module_sections.clear();
}

void EffectModuleSection::handlePopupResult(int result) {
    //std::vector<vital::ModulationConnection*> connections = getConnections();
    // if (result == 1) {
    //     juce::ValueTree t(IDs::EffectMODULE);
    //     t.setProperty(IDs::type, "osc", nullptr);
    //     list.appendChild(t, nullptr);
    if (result == 1) {
        juce::ValueTree t(IDs::EFFECTMODULE);
        t.setProperty(IDs::type, "filt", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    }
    else if (result == 2) {
        juce::ValueTree t(IDs::EFFECTMODULE);
        t.setProperty(IDs::type, "delay", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    }
    // } else if (result == 3) {
    //     juce::ValueTree t(IDs::EffectMODULE);
    //     t.setProperty(IDs::type, "string", nullptr);
    //     list.appendChild(t, nullptr);
    // }

}


void EffectModuleSection::setEffectPositions() {
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    // No vertical gap between stacked FX modules (FX-local; SoundModuleSection unaffected).
    int padding = 0;
    int start_y = 0;

    for (int i = 0; i < module_sections.size(); ++i) {
        // If placeholder is before this section, shift this section down
        int y = start_y;
        if ( i == placeholderIndex) {
            y += placeholderHeight;
        }

        // Size each FX module from its contained view's dynamic row layout instead of the
        // full viewport height. Set bounds once first so the FX view gets its width and can
        // compute a width-dependent row count, then read the preferred height and apply it.
        // (FX-local: SoundModuleSection sizes its modules separately.)
        module_sections[i]->setBounds(0, y, getWidth(), module_sections[i]->height);
        module_sections[i]->height = module_sections[i]->getPreferredHeight();
        module_sections[i]->setDrawBottomSeparator(i + 1 < module_sections.size());
        module_sections[i]->setBounds(0, y, getWidth(), module_sections[i]->height);
        start_y = y + module_sections[i]->height + padding;
    }

    // Update container height to include placeholder
    int totalHeight = start_y;
    if (placeholderIndex >= 0) {
        totalHeight += placeholderHeight + padding;
    }

    container_->setBounds(0, getTitleWidth(), viewport_.getWidth(), totalHeight);
    viewport_.setViewPosition(viewport_.getViewPosition());  // preserve scroll
}


PopupItems EffectModuleSection::createPopupMenu() {
    PopupItems options;
    options.addItem(1, "add filt");
    options.addItem(2, "add delay");
    return options;
}

void EffectModuleSection::buttonClicked(juce::Button* button) {
    if (button == add_effect_button_.get()) {
        hidePopupDisplay(true);
        showPopupSelector(add_effect_button_.get(), add_effect_button_->getLocalBounds().getCentre(),
                          createPopupMenu(), [this](int selection) { handlePopupResult(selection); });
        return;
    }

    ModulesInterface<ProcessorBase>::buttonClicked(button);
}

void EffectModuleSection::mouseEnter(const juce::MouseEvent& event) {
    if (event.eventComponent == add_effect_button_.get()) {
        showPopupDisplay(add_effect_button_.get(), "Click to add effect",
                         juce::BubbleComponent::left, true);
    }
}

void EffectModuleSection::mouseExit(const juce::MouseEvent& event) {
    if (event.eventComponent == add_effect_button_.get())
        hidePopupDisplay(true);
}


std::map<std::string, SynthSlider *> EffectModuleSection::getAllSliders() {
    return container_->getAllSliders();
}

void EffectModuleSection::moduleAdded(ProcessorBase *newModule) {
    auto module_section = std::make_unique<ModuleSection>(newModule->state,std::move (newModule->createEditor()), undo);
    module_section->height = 300;
    module_section->onDragMove = [this](ModuleSection* dragged, juce::Rectangle<int> bounds) {
        int midY = bounds.getCentreY();

        int targetIndex = 0;
        int currModuleSection;
        for (int i = 0; i < (int)module_sections.size(); ++i) {
            if (module_sections[i].get() ==  dragged) {
                currModuleSection = 0;
                break;
            }
            // targetIndex = i + 1;
        }
        for (int i = 0; i < (int)module_sections.size(); ++i) {
            if (midY < module_sections[i]->getBounds().getCentreY()) {
                targetIndex = i;
                break;
            }
            targetIndex = i + 1;
        }


        placeholderHeight = dragged->height;
        reorderTargetIndex = targetIndex;
        // // Update layout to show gap
        if(placeholderIndex!= targetIndex) {
            placeholderIndex = targetIndex;
            auto it = std::find_if(module_sections.begin(), module_sections.end(),
                              [dragged](auto& p) { return p.get() == dragged; });

            if (it != module_sections.end()) {
                auto target_it = module_sections.begin() + reorderTargetIndex;

                if (it < target_it) std::rotate(it, it + 1, target_it);
                else std::rotate(target_it, it, it + 1);

                dragged->setAlwaysOnTop(false);
            }

            // Clear temporary placeholder
            // placeholderIndex = -1;
            placeholderHeight = dragged->height;

            // Finalize positions
            // setEffectPositions();
            if (getWidth() <= 0 || getHeight() <= 0)
                return;

            // No vertical gap between stacked FX modules (see setEffectPositions).
            int padding = 0;
            int start_y = 0;

            for (int i = 0; i < module_sections.size(); ++i) {
                // If placeholder is before this section, shift this section down
                int y = start_y;
                if ( i == placeholderIndex) {
                    start_y += placeholderHeight;
                    continue;
                }

                module_sections[i]->setBounds(0, y, getWidth(), module_sections[i]->height);
                start_y = y + module_sections[i]->height + padding;
            }

            // Update container height to include placeholder
            int totalHeight = start_y;
            if (placeholderIndex >= 0) {
                totalHeight += placeholderHeight + padding;
            }

            container_->setBounds(0, getTitleWidth(), viewport_.getWidth(), totalHeight);
            viewport_.setViewPosition(viewport_.getViewPosition());  // preserve scroll
            // setEffectPositions();
        }


        dragged->setAlwaysOnTop(true);
        reorderTargetIndex = targetIndex;
    };


    module_section->onDragEnd = [this](ModuleSection* dragged, juce::Rectangle<int>) {
        auto it = std::find_if(module_sections.begin(), module_sections.end(),
                               [dragged](auto& p) { return p.get() == dragged; });

        if (it != module_sections.end()) {
            auto target_it = module_sections.begin() + reorderTargetIndex;

            if (it < target_it) std::rotate(it, it + 1, target_it);
            else std::rotate(target_it, it, it + 1);

            dragged->setAlwaysOnTop(false);
        }

        // Clear temporary placeholder
        reorderTargetIndex = -1;
        placeholderHeight = 0;

        // Finalize positions
        setEffectPositions();
    };

    { juce::ScopedLock lock(open_gl_critical_section_);
        container_->addSubSection(module_section.get());
    }
    module_section->setInterceptsMouseClicks(true, true);
    parentHierarchyChanged();
    //int height_to_add  = module_section->height;
    module_sections.emplace_back(std::move(module_section));


    if (!getLocalBounds().isEmpty()) {
        //this->setSize(getWidth(),getHeight() + height_to_add);
        resized();
    }

    for (auto listener: listeners_) {
        listener->added();
    }
    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    for (auto sub : sub_sections_) {
            OpenGlComponent::setScissorBounds(sub, viewport_.getLocalBounds(), *interface->getOpenGlWrapper());
            for (auto slider : sub->all_sliders_) {
                //slider.second->setScissor(this, open_gl);
                slider.second->setScissorComponent(&viewport_);
            }
            for (auto component : sub->open_gl_components_) {
                component->setScissorComponent(&viewport_);
            }
        }
        container_->setScissorComponent(&viewport_);
        for (auto component : container_->open_gl_components_) {
            component->setScissorComponent(&viewport_);
        }
        for (auto sub : container_->sub_sections_) {
            for (auto view : sub->sub_sections_) {
                for (auto component : view->open_gl_components_) {
                    component->setScissorComponent(&viewport_);
                }
            }
            for (auto component : sub->open_gl_components_) {
                component->setScissorComponent(&viewport_);
            }
        }

}
void EffectModuleSection::resized() {
    //ModulesInterface::resized();
    static constexpr float kEffectOrderWidthPercent = 0.2f;
    static constexpr int kHeaderSidePadding = 6;
    static constexpr int kHeaderControlGap = 4;
    static constexpr int kRoutingControlHeight = 14;
    static constexpr int kAddButtonSize = 34;
    static constexpr int kHeaderControlsHeight = kAddButtonSize;

    ScopedLock lock(open_gl_critical_section_);

    const int title_width = static_cast<int>(getTitleWidth());
    const int header_height = title_width + kHeaderControlsHeight;

    int order_width = getWidth() * kEffectOrderWidthPercent;
    //    effect_order_->setBounds(0, 0, order_width, getHeight());
    //    effect_order_->setSizeRatio(size_ratio_);
    int large_padding = findValue(Skin::kLargePadding);
    int shadow_width = getComponentShadowWidth();
    int viewport_x = 0 + large_padding - shadow_width;
    int viewport_width = getWidth() - viewport_x - large_padding + 2 * shadow_width;
    auto area = getLocalBounds();
    auto header = area.removeFromTop(30);
    toggle_button_->setBounds(0,0,getTitleWidth(),getTitleWidth());
    if (isExpanded()) {
        viewport_.setVisible(true);
        container_->setVisible(true);
        viewport_.setBounds(0, header_height, getWidth(), std::max(0, getHeight() - header_height - 2));
        setEffectPositions();
        scroll_bar_->setBounds(getWidth() - large_padding, header_height + large_padding,
                               large_padding - 2,
                               std::max(0, getHeight() - header_height - (large_padding + 2 * shadow_width)));
        scroll_bar_->setColor(ShaderColors::kEffectTextColor);

        // Clip every live child to the FX viewport while scrolling, matching
        // SoundModuleSection::resized(). Without this, scrolled FX child content is not
        // bounded to the visible viewport and bleeds into the section below.
        container_->setScissorComponent(&viewport_);
        for (auto component : container_->open_gl_components_)
            component->setScissorComponent(&viewport_);
        for (auto sub : container_->sub_sections_) {
            sub->setScissorComponent(&viewport_);
            for (auto slider : sub->all_sliders_)
                slider.second->setScissorComponent(&viewport_);
            for (auto component : sub->open_gl_components_)
                component->setScissorComponent(&viewport_);
            for (auto view : sub->sub_sections_) {
                view->setScissorComponent(&viewport_);
                for (auto slider : view->all_sliders_)
                    slider.second->setScissorComponent(&viewport_);
                for (auto component : view->open_gl_components_)
                    component->setScissorComponent(&viewport_);
            }
        }
    }
    else
    {
        viewport_.setVisible(false);
        container_->setVisible(false);
        viewport_.setBounds(0,0,0,0);
        container_->setBounds(0,0,0,0);
    }

    SynthSection::resized();
    redoBackgroundImage();
    //ooter_body->setBounds(0,getHeight()-1, getWidth(), getTitleWidth());
    footer_body->setRounding(findValue(Skin::kBodyRounding));
    footer_body->setColor(findColour(Skin::kBody, true));

    header_body_->setBounds(0, 0, getWidth(), header_height);
    header_body_->setColor(findColour(Skin::kBodyHeading, true));
    header_title_->setBounds(0, 0, getWidth(), title_width);
    header_title_->setText(getName());
    header_title_->setTextSize(size_ratio_ * 14.0f);
    header_title_->setColor(findColour(Skin::kHeadingText, true));

    const int controls_y = title_width;
    const int controls_height = kHeaderControlsHeight;
    const int available_width = std::max(0, getWidth() - 2 * kHeaderSidePadding);
    const int button_size = std::min(kAddButtonSize, available_width);
    const int gap = available_width > button_size ? kHeaderControlGap : 0;
    const int combo_width = std::max(0, available_width - button_size - gap);
    const int control_height = std::min(kRoutingControlHeight, controls_height);
    const int control_y = controls_y + (controls_height - control_height) / 2;

    routing_combo_box_->setBounds(kHeaderSidePadding, control_y, combo_width, control_height);
    const int button_x = getWidth() - kHeaderSidePadding - button_size;
    const int button_y = controls_y + (controls_height - button_size) / 2;
    add_effect_button_->setBounds(button_x, button_y, button_size, button_size);
    add_effect_button_background_->setBounds(add_effect_button_->getBounds().reduced(4));
    add_effect_button_->setColour(Skin::kIconButtonOff, findColour(Skin::kIconButtonOff, true));
    add_effect_button_->setColour(Skin::kIconButtonOffHover, findColour(Skin::kIconButtonOffHover, true));
    add_effect_button_->setColour(Skin::kIconButtonOffPressed, findColour(Skin::kIconButtonOffPressed, true));

    const auto border_bounds = getLocalBounds();
    const auto border_color = findColour(Skin::kBorder, true);
    // The rounded-border shader's zero-radius inner mask has non-zero alpha across the
    // quad interior. A half-unit floor is the shader's antialias cutoff and keeps a
    // square-looking zero-radius skin border transparent away from its perimeter.
    const auto border_rounding = std::max(0.5f, findValue(Skin::kBodyRounding));
    for (auto* border : { border_overlay_.get(), border_overlay_second_pass_.get() }) {
        border->setBounds(border_bounds);
        border->setColor(border_color);
        border->setRounding(border_rounding);
        border->setThickness(1.0f, true);
    }
}
void EffectModuleSection::removeModule(ProcessorBase *newModule) {
    // Find exactly the one module whose state matches. find_if (vs non-stable
    // std::partition) does not reorder the surviving modules.
    auto it = [&]() {
        juce::ScopedLock lock(this->open_gl_critical_section_);
        return std::find_if(module_sections.begin(), module_sections.end(),
                            [newModule](auto& section) {
                                return section->state == newModule->state;
                            });
    }();

    // Guard: if not found (e.g. double-removal), dereferencing end() is UB.
    if (it == module_sections.end())
        return;

    ModuleSection* section = it->get();

    // Make invisible before any rebake: paintChildrenBackgrounds skips invisible
    // children, so the removed module is excluded from the scroll background.
    section->setVisible(false);

    // Move ownership out of module_sections into a strong keep-alive, then erase the
    // slot and remove the subsection from the container -- all on the message thread
    // under open_gl_critical_section_, so container_->sub_sections_ and the slider maps
    // are mutated consistently with the renderer/resized() (which hold the same lock).
    // removeSubSection is CPU-only (no GL calls).
    std::shared_ptr<ModuleSection> keep_alive;
    {
        juce::ScopedLock lock(this->open_gl_critical_section_);
        keep_alive = std::shared_ptr<ModuleSection>(std::move(*it));
        module_sections.erase(it);
        this->container_->removeSubSection(keep_alive.get());
    }

    // Notify listeners (modulation rebuild is deferred/coalesced) and reflow. The
    // container is already consistent, so resized()/redoBackgroundImage see valid state.
    for (auto listener : listeners_)
        listener->removed();
    resized();

    // Async GL-only cleanup: free GL resources on the GL thread (non-blocking, so the
    // message thread is never parked), then drop the keep-alive back on the message
    // thread so ~ModuleSection() (a JUCE Component) runs there. The lambda captures
    // keep_alive only and does NOT mutate container_/sub_sections_.
    auto *_parent = findParentComponentOfClass<SynthGuiInterface>();
    _parent->getOpenGlWrapper()->context.executeOnGLThread([keep_alive](juce::OpenGLContext &openGLContext) {
        keep_alive->destroyOpenGlComponents(openGLContext);
        juce::MessageManager::callAsync([keep_alive]() mutable {
            keep_alive.reset();
        });
    }, false);
}

void EffectModuleSection::moduleListChanged() {
}
// void EffectModuleSection::renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) {
//     ScopedLock lock(open_gl_critical_section_);
//
//     OpenGlComponent::setViewPort(&viewport_, open_gl);
//
//     float image_width = background_.getImageWidth(); //electrosynth::utils::nextPowerOfTwo(background_.getImageWidth());
//     float image_height =background_.getImageHeight(); // electrosynth::utils::nextPowerOfTwo(background_.getImageHeight());
//     int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
//     float width_ratio = image_width / (container_->getWidth() * mult);
//     float height_ratio = image_height / (viewport_.getHeight() * mult);
//    // DBG(viewport_.getViewPositionY());
//     float y_offset =(2.0f * viewport_.getViewPositionY()) /viewport_.getHeight();
//
//     // --- Debug output ---
//     // DBG("image_width: " + juce::String(image_width));
//     // DBG("image_height: " + juce::String(image_height));
//     // DBG("mult (scale factor): " + juce::String(mult));
//     // DBG("container width: " + juce::String(container_->getWidth()));
//     // DBG("viewport height: " + juce::String(viewport_.getHeight()));
//     // DBG("width_ratio: " + juce::String(width_ratio));
//     // DBG("height_ratio: " + juce::String(height_ratio));
//     // DBG("viewport Y offset: " + juce::String(viewport_.getViewPositionY()));
//     // DBG("computed y_offset: " + juce::String(y_offset));
//     //
//     background_.setTopLeft(-1.0f, 1.0f+ y_offset);
//     background_.setTopRight(-1.0f + 2.0f * width_ratio,  1.0f+y_offset);
//     background_.setBottomLeft(-1.0f, 1.0f - 2.0f * height_ratio + y_offset);
//     background_.setBottomRight(-1.0f + 2.0f * width_ratio, 1.0f - 2.0f * height_ratio + y_offset);
//     background_.setColor(Colours::white);
//     background_.drawImage(open_gl);
//
//     OpenGlComponent::setScissorBounds(this, getLocalBounds(),open_gl);
//     //TODO: clean up. this is to check here becuase I can do this creationlazy do better
//     for (auto sub : sub_sections_) {
//         OpenGlComponent::setScissorBounds(sub, viewport_.getLocalBounds(), open_gl);
//         for (auto slider : sub->all_sliders_) {
//             //slider.second->setScissor(this, open_gl);
//             slider.second->setScissorComponent(&viewport_);
//         }
//         for (auto component : sub->open_gl_components_) {
//             component->setScissorComponent(&viewport_);
//         }
//     }
//     container_->setScissorComponent(&viewport_);
//     for (auto component : container_->open_gl_components_) {
//         component->setScissorComponent(&viewport_);
//     }
//     for (auto sub : container_->sub_sections_) {
//         for (auto view : sub->sub_sections_) {
//             for (auto component : view->open_gl_components_) {
//                 component->setScissorComponent(&viewport_);
//             }
//         }
//         for (auto component : sub->open_gl_components_) {
//             component->setScissorComponent(&viewport_);
//         }
//     }
// }

// void EffectModuleSection::mouseDown(const juce::MouseEvent &e) {
//     // mouse_down_y_ = e.y;
//     //
//     // for (int i =0; i< module_sections.size(); ++i) {
//     //     if ( module_sections[i]->hover_) {
//     //         currently_dragged_ = module_sections[i].get();
//     //         last_dragged_index_ =i;
//     //     }
//     //
//     // }
//     // dragged_starting_y_ = currently_dragged_->getY();
//     // currently_dragged_->setAlwaysOnTop(true);
// }

// void EffectModuleSection::mouseDrag(const MouseEvent& e) {
//     if (currently_dragged_ == nullptr)
//         return;
//
//     int delta_y = e.y - mouse_down_y_;
//     int clamped_y = electrosynth::utils::iclamp(dragged_starting_y_ + delta_y, 0,
//                                          getHeight() - currently_dragged_->getHeight());
//     currently_dragged_->setTopLeftPosition(currently_dragged_->getX(), clamped_y);
//
//     int next_index = ;
//     if (next_index != last_dragged_index_) {
//         moveEffect(last_dragged_index_, next_index);
//         last_dragged_index_ = next_index;
//     }
void EffectModuleSection::redoBackgroundImage() {
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    Colour background = findColour(Skin::kBackground, true);
    int height = std::max(container_->getHeight(), static_cast<int>(viewport_.getHeight()));
    if (height == 0)
        height = getHeight();
    int width = std::max(container_->getWidth(), getWidth());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;

    Image background_image = Image(Image::ARGB, width * mult, height * mult, true);

    Graphics background_graphics(background_image);
    background_graphics.addTransform(AffineTransform::scale(mult));
    background_graphics.fillAll(background);
    if (isExpanded())
        container_->paintBackground(background_graphics);
    background_.setOwnImage(background_image);
}

void EffectModuleSection::paintBackground(Graphics &g) {
    g.setColour(findColour(Skin::kBody, true));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), findValue(Skin::kBodyRounding));
    g.setColour(findColour(Skin::kBorder, true));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), findValue(Skin::kBodyRounding), 1.0f);

    paintBody(g);

    redoBackgroundImage();
}
