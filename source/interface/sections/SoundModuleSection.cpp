//
// Created by Davis Polito on 11/19/24.
//

#include "SoundModuleSection.h"
#include "../../synthesis/framework/Processors/OscillatorModuleProcessor.h"
#include "ModuleSection.h"
#include "synth_gui_interface.h"
#include "Processors/ProcessorBase.h"
#include "modulation_manager.h"
#include "synth_base.h"

SoundModuleSection::SoundModuleSection(ModulationManager *m,
                                       ModuleList<ProcessorBase> &module_list,const juce::ValueTree &v, juce::UndoManager& um) :
ModulesInterface( module_list), footer_body(new OpenGlQuad(Shaders::kRoundedRectangleFragment)), state(v), undo(um)
{
    // scroll_bar_ = std::make_unique<OpenGlScrollBar>();
    // addAndMakeVisible(scroll_bar_.get());
    // addOpenGlComponent(scroll_bar_->getGlComponent());
    addOpenGlComponent(footer_body);

    setLookAndFeel(DefaultLookAndFeel::instance());
    // scroll_bar_->addListener(this);
    viewport_.setScrollBarPosition(false, false); //use this to determine viewport scroll type in effectsviewport
    viewport_.setScrollBarsShown(false, false, false, false);

    addListener(m);
    for (auto obj : list) {
        SoundModuleSection::moduleAdded(obj);
    }
    setSidewaysHeading(false);
    setName("Sound Module");

    exit_button_ = std::make_unique<OpenGlShapeButton>("Exit");
    addAndMakeVisible(exit_button_.get());
    addOpenGlComponent(exit_button_->getGlComponent());
    exit_button_->addListener(this);
    exit_button_->setShape(Paths::exitX());

    //can only use because i know that this will work
    //i.e. i know the type that router will return
    auto baseEditor = module_list.router_->createEditor();
    routing_view_ = std::unique_ptr<RoutingView>(static_cast<RoutingView*>(baseEditor.release()));
    addSubSection(routing_view_.get());
    routing_view_->setAlwaysOnTop(true);
}

SoundModuleSection::~SoundModuleSection() {
   module_sections.clear();
}
void SoundModuleSection::redoBackgroundImage()
{
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
    container_->paintBackground(background_graphics);
    paintChildBackground(background_graphics,routing_view_.get());
    background_.setOwnImage(background_image);
}

void SoundModuleSection::handlePopupResult(int result) {
    //std::vector<vital::ModulationConnection*> connections = getConnections();
    if (result == 1) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "osc", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    } else if (result == 2) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "filt", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    } else if (result == 3)
    {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "string", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    } else if (result == 4) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "softclip", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    }

    //    if (result == kArmMidiLearn)
    //        synth->armMidiLearn(getName().toStdString());
    //    else if (result == kClearMidiLearn)
    //        synth->clearMidiLearn(getName().toStdString());
    //    else if (result == kDefaultValue)
    //        setValue(getDoubleClickReturnValue());
    //    else if (result == kManualEntry)
    //        showTextEntry();
    //    else if (result == kClearModulations) {
    //        for (vital::ModulationConnection* connection : connections) {
    //            std::string source = connection->source_name;
    //            synth_interface_->disconnectModulation(connection);
    //        }
    //        notifyModulationsChanged();
    //    }
    //    else if (result >= kModulationList) {
    //        int connection_index = result - kModulationList;
    //        std::string source = connections[connection_index]->source_name;
    //        synth_interface_->disconnectModulation(connections[connection_index]);
    //        notifyModulationsChanged();
    //    }
}


void SoundModuleSection::setEffectPositions() {
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    int padding = getPadding();
    int large_padding = findValue(Skin::kLargePadding);
    int shadow_width = getComponentShadowWidth();
    int start_x = large_padding - shadow_width;
    int effect_width = getWidth() - start_x - large_padding;
    int knob_section_height = getKnobSectionHeight();
    int widget_margin = findValue(Skin::kWidgetMargin);
    int effect_height =  knob_section_height - widget_margin;
    int y = 0;//+ getTitleWidth();

    juce::Point<int> position = viewport_.getViewPosition();
    // DBG("position viewport: x: " + juce::String(position.getX()) + "y: " + juce::String(position.getY()));
    //DBG("shadwo width: " + String(shadow_width));
    for (auto &section: module_sections) {
        section->setBounds(0, y, effect_width, section->height);
        y += (section->height +padding);
    }
    container_->setBounds(0,getTitleWidth(), viewport_.getWidth(), y+padding);
    viewport_.setViewPosition(position);

    for (Listener *listener: listeners_)
        listener->effectsMoved();
    //DBG("container Height " + String(container_->getHeight()));
    //DBG("viewport Height " + String(viewport_.getWidth()));
    // container_->setScrollWheelEnabled(container_->getHeight() <= viewport_.getHeight());
    // setScrollBarRange();
    repaintBackground();
    height = y+padding + getTitleWidth()*2;
}

PopupItems SoundModuleSection::createPopupMenu() {
    PopupItems options;
    options.addItem(1, "add osc");
    options.addItem(2, "add filt");
    options.addItem(3, "add string");
    options.addItem(4, "soft clip");
    return options;
}


std::map<std::string, SynthSlider *> SoundModuleSection::getAllSliders() {
    return container_->getAllSliders();
}

void SoundModuleSection::moduleAdded(ProcessorBase *newModule) {
    auto module_section = std::make_unique<ModuleSection>(newModule->state,std::move (newModule->createEditor()), undo);
    { juce::ScopedLock lock(open_gl_critical_section_);
        container_->addSubSection(module_section.get());
    }
    module_section->setInterceptsMouseClicks(false, true);
    parentHierarchyChanged();
    int height_to_add  = module_section->height;
    module_sections.emplace_back(std::move(module_section));


    if (!getLocalBounds().isEmpty()) {
        int padding = getPadding();
        this->setSize(getWidth(),getHeight() + height_to_add + padding);
        resized();
    }

    for (auto listener: listeners_) {
        listener->added();
    }

}
void SoundModuleSection::resized() {
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
        viewport_.setBounds(0,getTitleWidth(),getWidth(),getHeight()-(getTitleWidth()*2)); //getHeight()-getTitleWidth() - (large_padding + 20 * shadow_width));
        setEffectPositions();
        // scroll_bar_->setBounds(getWidth() - large_padding + 1, getTitleWidth() + large_padding, large_padding - 2, getHeight() -getTitleWidth()-(large_padding + 2 * shadow_width));
        // scroll_bar_->setColor(findColour(Skin::kLightenScreen, true));


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
    routing_view_->setBounds(getLocalBounds().getRight() - 200, viewport_.getBottom(), 200, getTitleWidth());
    exit_button_->setBounds(getLocalBounds().getRight() - 50,0, 25,25);
}
void SoundModuleSection::removeModule(ProcessorBase *newModule) {
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
this->setSize(getWidth(),getHeight() - height_to_remove);
    resized();
    for(auto listener : listeners_)
    {
        listener->removed();
    }
    redoBackgroundImage();

}

void SoundModuleSection::moduleListChanged() {
}
void SoundModuleSection::buttonClicked(juce::Button *button) {
    ModulesInterface<ProcessorBase>::buttonClicked(button);
    if (button == exit_button_.get()) {
        this->setVisible(false);
        //DBG("state " state.getParent())
        undo.beginNewTransaction();
        state.getParent().removeChild(state,&undo);
    }
}

// void SoundModuleSection::renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) {
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

// void SoundModuleSection::mouseDown(const juce::MouseEvent &e) {
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

// void SoundModuleSection::mouseDrag(const MouseEvent& e) {
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
