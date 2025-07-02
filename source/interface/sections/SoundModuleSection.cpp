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
                                       ModuleList<ProcessorBase> &module_list) : ModulesInterface( module_list), footer_body(new OpenGlQuad(Shaders::kRoundedRectangleFragment)) {
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
        SoundModuleSection::moduleAdded(obj);
    }
    setSidewaysHeading(false);
    setName("section");



}

SoundModuleSection::~SoundModuleSection() {
   module_sections.clear();
}

void SoundModuleSection::handlePopupResult(int result) {
    //std::vector<vital::ModulationConnection*> connections = getConnections();
    if (result == 1) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "osc", nullptr);
        list.appendChild(t, nullptr);
    } else if (result == 2) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "filt", nullptr);
        list.appendChild(t, nullptr);
    } else if (result == 3) {
        juce::ValueTree t(IDs::SOUNDMODULE);
        t.setProperty(IDs::type, "string", nullptr);
        list.appendChild(t, nullptr);
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
        section->setBounds(0, y, effect_width, effect_height);
        y += (effect_height +padding);
    }
    container_->setBounds(0,getTitleWidth(), viewport_.getWidth(), y - padding + effect_height * 2);
    viewport_.setViewPosition(position);

    for (Listener *listener: listeners_)
        listener->effectsMoved();
    //DBG("container Height " + String(container_->getHeight()));
    //DBG("viewport Height " + String(viewport_.getWidth()));
    container_->setScrollWheelEnabled(container_->getHeight() <= viewport_.getHeight());
    setScrollBarRange();
    repaintBackground();
}

PopupItems SoundModuleSection::createPopupMenu() {
    PopupItems options;
    options.addItem(1, "add osc");
    options.addItem(2, "add filt");
    options.addItem(3, "add string");
    return options;
}


std::map<std::string, SynthSlider *> SoundModuleSection::getAllSliders() {
    return container_->getAllSliders();
}

void SoundModuleSection::moduleAdded(ProcessorBase *newModule) {
    auto module_section = std::make_unique<ModuleSection>(newModule->state,std::move (newModule->createEditor()));
    { juce::ScopedLock lock(open_gl_critical_section_);
        container_->addSubSection(module_section.get());
    }
    module_section->setInterceptsMouseClicks(false, true);
    parentHierarchyChanged();
    module_sections.emplace_back(std::move(module_section));

        for (auto listener: listeners_) {
            listener->added();
        }

    resized();
}
void SoundModuleSection::resized() {
    ModulesInterface::resized();
    //ooter_body->setBounds(0,getHeight()-1, getWidth(), getTitleWidth());
    footer_body->setRounding(findValue(Skin::kBodyRounding));
    footer_body->setColor(findColour(Skin::kBody, true));
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



    module_sections.erase(it);
    DBG("deletesection");

    for(auto listener : listeners_)
    {
        listener->removed();
    }
    resized();
}

void SoundModuleSection::moduleListChanged() {
}
