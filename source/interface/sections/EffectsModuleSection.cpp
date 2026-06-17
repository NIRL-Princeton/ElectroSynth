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
    setName("section");

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

    int padding = getPadding();
    int start_y = 0;

    for (int i = 0; i < module_sections.size(); ++i) {
        // If placeholder is before this section, shift this section down
        int y = start_y;
        if ( i == placeholderIndex) {
            y += placeholderHeight;
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
}


PopupItems EffectModuleSection::createPopupMenu() {
    PopupItems options;
    options.addItem(1, "add filt");
    options.addItem(2, "add delay");
    return options;
}


std::map<std::string, SynthSlider *> EffectModuleSection::getAllSliders() {
    return container_->getAllSliders();
}

void EffectModuleSection::moduleAdded(ProcessorBase *newModule) {
    auto module_section = std::make_unique<ModuleSection>(newModule->state,std::move (newModule->createEditor()), undo);
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

            int padding = getPadding();
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

    ScopedLock lock(open_gl_critical_section_);

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
        viewport_.setBounds(0,getTitleWidth(),getWidth(),getHeight()-getTitleWidth()*2); //getHeight()-getTitleWidth() - (large_padding + 20 * shadow_width));
        setEffectPositions();
        scroll_bar_->setBounds(getWidth() - large_padding + 1, getTitleWidth() + large_padding, large_padding - 2, getHeight() -getTitleWidth()-(large_padding + 2 * shadow_width));
        scroll_bar_->setColor(findColour(Skin::kLightenScreen, true));


    }
    else
    {
        viewport_.setBounds(0,0,0,0);
        container_->setBounds(0,0,0,0);
    }

    SynthSection::resized();
    //ooter_body->setBounds(0,getHeight()-1, getWidth(), getTitleWidth());
    footer_body->setRounding(findValue(Skin::kBodyRounding));
    footer_body->setColor(findColour(Skin::kBody, true));
}
void EffectModuleSection::removeModule(ProcessorBase *newModule) {
    DBG(newModule->state.getProperty(IDs::uuid).toString());
     DBG("prepartoremoeve");
    // decltype(module_sections)::iterator it;
    // {
    //     juce::ScopedLock(this->open_gl_critical_section_);
    //     it = std::remove_if(module_sections.begin(), module_sections.end(),
    //                         [newModule](auto& section) {
    //                             return section->state == newModule->state;
    //                         });
    // }
    // leaving this here as its another way to accomplish this task
     auto it = [&]() {
         juce::ScopedLock lock(this->open_gl_critical_section_);
         return std::partition(module_sections.begin(), module_sections.end(),
                               [newModule](auto& section) {
                                   return section->state != newModule->state;
                               });
     }();



        it->get()->setVisible(false);


            auto *_parent = findParentComponentOfClass<SynthGuiInterface>();
            _parent->getOpenGlWrapper()->context.executeOnGLThread([this, it](juce::OpenGLContext &openGLContext) {


                auto a = it->get();
                a->destroyOpenGlComponents(openGLContext);
                this->container_->removeSubSection(a);
                DBG("delete");
                },true);


    int height_to_remove = it->get()->height;
    module_sections.erase(it);
    DBG("deletesection");

    for(auto listener : listeners_)
    {
        listener->removed();
    }
    // this->setSize(getWidth(),getHeight() - height_to_remove);
    resized();
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
void EffectModuleSection::paintBackground(Graphics &g) {

    g.setColour(findColour(Skin::kBody, true));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), findValue(Skin::kBodyRounding));
    g.setColour(findColour(Skin::kBorder, true));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), findValue(Skin::kBodyRounding), 1.0f);

    // paintContainer(g);
    paintBody(g);
    paintHeadingText(g);

    // paintChildrenBackgrounds(g);
    paintBorder(g);
    // paintChildBackground(g,container_.get());
    Colour background = findColour(Skin::kBackground, true);

    int height = std::max(container_->getHeight(),static_cast<int> (viewport_.getHeight()));
    if (height == 0)
        height = getHeight();
    int width = std::max(container_->getWidth(), getWidth());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
    Image background_image = Image(Image::ARGB, width * mult, height * mult, true);

    Graphics background_graphics(background_image);
    background_graphics.addTransform(AffineTransform::scale(mult));
    background_graphics.fillAll(background);
    // container_->paintBackground(background_graphics);
    background_.setOwnImage(background_image);
    // redoBackgroundImage();

}
