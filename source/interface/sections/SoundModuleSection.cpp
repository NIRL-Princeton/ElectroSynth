//
// Created by Davis Polito on 11/19/24.
//

// SoundModuleSection.cpp is the container/list for all sound modules. It owns the pop-up menu, creates/removes modules, stores
// module_sections, lays them out vertically, owns the viewport/container, and positions the routing gain/combobox over
// the first module header.

#include "SoundModuleSection.h"
#include "../../synthesis/framework/Processors/OscillatorModuleProcessor.h"
#include "ModuleSection.h"
#include "Processors/ProcessorBase.h"
#include "chowdsp_plugin_utils/Files/chowdsp_TweaksFile.h"
#include "mapping_manager.h"
#include "synth_base.h"
#include "synth_gui_interface.h"

SoundModuleSection::SoundModuleSection(MappingManager *m,  ModuleList<ProcessorBase> &module_list,const juce::ValueTree &v,
        juce::UndoManager& um) : mapping_manager_(m), ModulesInterface( module_list),
        footer_body(new OpenGlQuad(Shaders::kRoundedRectangleFragment)), state(v), undo(um) {

    setName("Sound Module");
    setSkinOverride (Skin::kSoundModule);

    header_body_ = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, "sound_module_header");
    header_body_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(header_body_, true);

    header_title_ = std::make_shared<PlainTextComponent>("sound_module_title", getName());
    header_title_->setFontType(PlainTextComponent::kLight);
    header_title_->setJustification(juce::Justification::centred);
    header_title_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(header_title_);

    addOpenGlComponent(footer_body);

    // setLookAndFeel(DefaultLookAndFeel::instance());

    addListener(m);

    for (auto obj : list) { SoundModuleSection::moduleAdded(obj); }

    exit_button_ = std::make_unique<OpenGlShapeButton>("Exit");
    addAndMakeVisible(exit_button_.get());
    addOpenGlComponent(exit_button_->getGlComponent());
    exit_button_->addListener(this);
    exit_button_->setShape(Paths::exitX());

    // Add modulator plus sign background
    add_button_background_ = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleFragment, "modulation_button_background_");
    add_button_background_->setInterceptsMouseClicks(false, false);
    add_button_background_->setRounding(5.0f);
    addOpenGlComponent (add_button_background_);
    // Add modulator plus sign and add listener
    add_to_module_button_ = std::make_unique<OpenGlShapeButton>("Add To This Sound Module");
    addAndMakeVisible (add_to_module_button_.get());
    addOpenGlComponent (add_to_module_button_->getGlComponent());
    add_to_module_button_->addListener (this);
    add_to_module_button_->setShape(Paths::plus (150));
    add_to_module_button_->addMouseListener (this, false);

    auto baseEditor = module_list.router_->createEditor();
    routing_view_ = std::unique_ptr<RoutingView>(static_cast<RoutingView*>(baseEditor.release()));
    addSubSection(routing_view_.get());
    routing_view_->setAlwaysOnTop(true);
}

SoundModuleSection::~SoundModuleSection() {
   module_sections.clear();
}

void SoundModuleSection::setSoundModuleIndex(int index) {
    sound_module_index_ = index;
    setName("Sound Module " + juce::String(sound_module_index_));
}

int SoundModuleSection::getCollapsedHeight() {
    return static_cast<int>(getTitleWidth());
}

int SoundModuleSection::getExpandedHeight() {
    const int padding = static_cast<int>(getPadding());
    const int header_height = static_cast<int>(getTitleWidth());
    static constexpr int kEmptyContentHeight = 54;
    int content_height = 0;

    for (auto& section : module_sections)
        content_height += section->refreshHeight() + padding;
    int empty_content_height = module_sections.empty() ? kEmptyContentHeight : 0;

    return content_height + 2 * header_height + empty_content_height + padding;
}

void SoundModuleSection::redoBackgroundImage() {
    int height = std::max(container_->getHeight(), viewport_.getHeight());
    if (height == 0) height = getHeight();
    const int width = std::max(container_->getWidth(), getWidth());
    const int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();

    Image background_image = Image(Image::ARGB, width * mult, height * mult, true);
    Colour background_color = findColour(Skin::kBackground, true);

    Graphics background_graphics(background_image);
    background_graphics.addTransform(AffineTransform::scale(mult));
    background_graphics.fillAll(background_color);
    if (isExpanded()) container_->paintBackground(background_graphics);
    background_.setOwnImage(background_image);
}

void SoundModuleSection::handlePopupResult(int result) {

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
    } else if (result == 3) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "string", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    } else if (result == 4) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "softclip", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    } else if (result == 5) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "noise", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    } else if (result == 6) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "sine", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    } else if (result == 7) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "sampHold", nullptr);
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
    if (getWidth() <= 0 || getHeight() <= 0) return;

    const int padding = findValue(Skin::kPadding);

    const int start_x = padding;
    const int effect_width = getWidth() - 2 * padding;
    int y = 0;

    const juce::Point<int> position = viewport_.getViewPosition();
    int oscillator_index = 1;
    int string_index = 1;
    int filter_index = 1;
    int soft_clip_index = 1;
    int noise_index = 1;
    int sine_index = 1;
    for (size_t index = 0; index < module_sections.size(); ++index) {
        auto& section = module_sections[index];
        const auto type = section->state.getProperty(IDs::type).toString();
        if (type == "osc")
            section->setName("Oscillator " + juce::String(sound_module_index_) + "." + juce::String(oscillator_index++));
        else if (type == "string")
            section->setName("String " + juce::String(sound_module_index_) + "." + juce::String(string_index++));
        else if (type == "filt")
            section->setName("Filter " + juce::String(sound_module_index_) + "." + juce::String(filter_index++));
        else if (type == "softclip")
            section->setName("Soft Clip " + juce::String(sound_module_index_) + "." + juce::String(soft_clip_index++));
        else if (type == "noise")
            section->setName("Noise " + juce::String(sound_module_index_) + "." + juce::String(noise_index++));
        else if (type == "sine")
            section->setName("Sine " + juce::String(sound_module_index_) + "." + juce::String(sine_index++));
        else if (type == "sampHold")
            section->setName("Sample & Hold " + juce::String(sound_module_index_) + "." + juce::String(sine_index++));

        const int section_height = section->refreshHeight(); // refresh height before positioning each module
        section->setDrawBottomSeparator(true);  //setDrawBottomSeparator(index + 1 < module_sections.size()); // add line separating modules
        section->setBounds(start_x, y, effect_width, section_height);
        y += (section_height + padding);
    }
    container_->setBounds(0, 0, viewport_.getWidth(), y + padding);
    viewport_.setViewPosition(position);

    for (Listener *listener: listeners_)
        listener->effectsMoved();
    repaintBackground();
    height = getExpandedHeight();
    if (getWidth() > 0 && getHeight() != height)
        setSize(getWidth(), height);
}

PopupItems SoundModuleSection::createPopupMenu() {
    PopupItems options;
    options.addItem(1, "add oscillator");
    options.addItem(2, "add filter");
    options.addItem(3, "add string");
    options.addItem(4, "add soft clip");
    options.addItem(5, "add noise");
    options.addItem(6, "add sine");
    options.addItem(7, "add Sample & Hold");
    return options;
}


std::map<std::string, SynthSlider *> SoundModuleSection::getAllSliders() {
    auto sliders = container_->getAllSliders();
    if (routing_view_ != nullptr) {
        auto routing_sliders = routing_view_->getAllSliders();
        sliders.insert(routing_sliders.begin(), routing_sliders.end());
    }
    return sliders;
}

void SoundModuleSection::paintBackground(juce::Graphics& g) {
    paintBorder(g);
    redoBackgroundImage();
}

void SoundModuleSection::moduleAdded(ProcessorBase *newModule) {
    auto module_section = std::make_unique<ModuleSection>(newModule->state, newModule->getAudioNodeDescriptor(),
    std::move (newModule->createEditor()), undo, mapping_manager_);
    module_section->setAreaSkinOverride(Skin::kSoundModule);
    {
        juce::ScopedLock lock(open_gl_critical_section_);
        container_->addSubSection(module_section.get());
    }
    module_section->applySkinFromTopLevel();
    module_section->setInterceptsMouseClicks(false, true);
    parentHierarchyChanged();
    module_section->refreshHeight();
    module_sections.emplace_back(std::move(module_section));


    if (!getLocalBounds().isEmpty()) { resized(); }

    for (auto listener: listeners_) { listener->added(); }

}
void SoundModuleSection::resized() {
    static auto button_size = findValue(Skin::kAddButtonSize);
    ScopedLock lock(open_gl_critical_section_);

    const int width = getWidth();
    const int title_width = getTitleWidth();

    toggle_button_->setBounds(0,0,title_width,title_width);

    // set Add Sound Module button bounds
    const int footer_y = getHeight() - title_width;
    const int button_x = (width - button_size) / 2;
    const int button_y = footer_y - (title_width * 2.5 - button_size);

    add_to_module_button_->setBounds( button_x, button_y, button_size, button_size);
    add_button_background_->setBounds(add_to_module_button_->getBounds().reduced(5));
    add_button_background_->setColor(findColour(Skin::kBorder, true));
    add_to_module_button_->setColour(Skin::kIconButtonOff,findColour(Skin::kIconButtonOff, true));
    add_to_module_button_->setColour(Skin::kIconButtonOffHover,findColour(Skin::kIconButtonOffHover, true));
    add_to_module_button_->setColour(Skin::kIconButtonOffPressed,findColour(Skin::kIconButtonOffPressed, true));

    if (isExpanded()) {

        viewport_.setVisible(true);
        container_->setVisible(true);

        add_button_background_->setVisible(true);
        add_to_module_button_->setVisible(true);

        viewport_.setBounds(0,title_width,width,getHeight()-(title_width*2));
        setEffectPositions();
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
    else {
        viewport_.setVisible(false);
        container_->setVisible(false);
        add_button_background_->setVisible(false);
        add_to_module_button_->setVisible(false);
        viewport_.setBounds(0,0,0,0);
        container_->setBounds(0,0,0,0);
        height = getCollapsedHeight();
        if (getWidth() > 0 && getHeight() != height)
            setSize(getWidth(), height);
    }

    SynthSection::resized();

    footer_body->setBounds(0,getHeight()-1, getWidth(), getTitleWidth());
    footer_body->setRounding(findValue(Skin::kBodyRounding));
    footer_body->setColor(findColour(Skin::kWidgetPrimary1, true));
    footer_body->setVisible(false); //******

    header_body_->setBounds(0, 0, getWidth(), title_width);
    header_body_->setColor(findColour(Skin::kBodyHeading, true));
    header_title_->setBounds(0, 0, getWidth(), title_width);
    header_title_->setText(getName());
    header_title_->setTextSize(size_ratio_ * 14.0f);
    header_title_->setColor(findColour(Skin::kHeadingText, true));

    static constexpr int kExitButtonSize = 25;
    static constexpr int kExitButtonRightOffset = 50;
    const int exit_x = getLocalBounds().getRight() - kExitButtonRightOffset;
    exit_button_->setBounds(exit_x,
                            (title_width - kExitButtonSize) / 2,
                            kExitButtonSize,
                            kExitButtonSize);



    if (routing_view_ != nullptr) {

        const auto local = getLocalBounds();
        routing_view_->setVisible(!module_sections.empty());
        routing_view_->setBounds(local.getX(), local.getY(), local.getWidth(), title_width);

    }
}



void SoundModuleSection::removeModule(ProcessorBase *newModule) {
    DBG("Remove module: " + newModule->state.getProperty(IDs::uuid).toString());

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

    module_sections.erase(it);
    DBG("deletesection");
    resized();
    for(auto listener : listeners_) {
        listener->removed();
    }
    redoBackgroundImage();
}

void SoundModuleSection::moduleListChanged() {
}

void SoundModuleSection::mouseEnter (const MouseEvent& event) {
    if (event.eventComponent == add_to_module_button_.get()) {
        showPopupDisplay(
            add_to_module_button_.get(),
            "Click to add to this sound module",
            juce::BubbleComponent::right,
            true);
    }
}

void SoundModuleSection::mouseExit (const MouseEvent& event) {
    if (event.eventComponent == add_to_module_button_.get())
        hidePopupDisplay(true);
}

void SoundModuleSection::buttonClicked(juce::Button *button) {
    ModulesInterface<ProcessorBase>::buttonClicked(button);

    if (button == add_to_module_button_.get()) {
        hidePopupDisplay(true);
        showPopupSelector (add_to_module_button_.get(), add_to_module_button_.get()->getLocalBounds().getCentre(),
            createPopupMenu(), [this] (int selection) {handlePopupResult (selection);});
    }

    if (button == exit_button_.get()) {
        this->setVisible(false);
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
