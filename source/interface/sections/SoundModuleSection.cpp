//
// Created by Davis Polito on 11/19/24.
//

#include "SoundModuleSection.h"
#include "../../synthesis/framework/Processors/OscillatorModuleProcessor.h"
#include "FilterModuleProcessor.h"
#include  "ModuleSections/ModuleSection.h"
#include "synth_gui_interface.h"
#include "Processors/ProcessorBase.h"
#include "modulation_manager.h"
#include "../../synthesis/framework/Processors/StringModuleProcessor.h"
#include "synth_base.h"
SoundModuleSection::SoundModuleSection(ValueTree &v, ModulationManager *m) : ModulesInterface<ModuleSection>(v)
{
    scroll_bar_ = std::make_unique<OpenGlScrollBar>();
    addAndMakeVisible(scroll_bar_.get());
    addOpenGlComponent(scroll_bar_->getGlComponent());
    scroll_bar_->addListener(this);
    viewport_.setScrollBarPosition(true,false);//use this to determine viewport scroll type in effectsviewport
    viewport_.setScrollBarsShown(false, false, true, false);

    factory.registerType<OscillatorModuleProcessor,electrosynth::SoundEngine*, juce::ValueTree, LEAF*>("osc");
    factory.registerType<FilterModuleProcessor, electrosynth::SoundEngine*,juce::ValueTree, LEAF*>("filt");
    factory.registerType<StringModuleProcessor,electrosynth::SoundEngine*, juce::ValueTree, LEAF*>("string");
    addListener(m);
}

SoundModuleSection::~SoundModuleSection()
{
    freeObjects();
}

void SoundModuleSection::handlePopupResult(int result) {

    //std::vector<vital::ModulationConnection*> connections = getConnections();
    if (result == 1 )
    {
        juce::ValueTree t(IDs::MODULE);
        t.setProperty(IDs::type, "osc", nullptr);
        parent.appendChild(t,nullptr);
    } else if (result == 2)
    {
        juce::ValueTree t(IDs::MODULE);
        t.setProperty(IDs::type, "filt", nullptr);
        parent.appendChild(t,nullptr);
    } else if (result == 3) {
        juce::ValueTree t(IDs::MODULE);
        t.setProperty(IDs::type, "string", nullptr);
        parent.appendChild(t,nullptr);
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
    int start_x = 0;
    int effect_width = getWidth() - start_x - large_padding;
    int knob_section_height = getKnobSectionHeight();
    int widget_margin = findValue(Skin::kWidgetMargin);
    int effect_height = 2 * knob_section_height - widget_margin;
    int y = 0;

    juce::Point<int> position = viewport_.getViewPosition();
   // DBG("position viewport: x: " + juce::String(position.getX()) + "y: " + juce::String(position.getY()));
    //DBG("shadwo width: " + String(shadow_width));
    for(auto& section : objects)
    {
        section->setBounds(shadow_width, y, effect_width, effect_height);
        y += effect_height + padding;
    }
    container_->setBounds(0, 0, viewport_.getWidth(), y - padding + effect_height * 2);
    viewport_.setViewPosition(position);

    for (Listener* listener : listeners_)
        listener->effectsMoved();
    DBG("container Height " + String(container_->getHeight()));
    DBG("viewport Height " + String(viewport_.getWidth()));
    container_->setScrollWheelEnabled(container_->getHeight() <= viewport_.getHeight());
    setScrollBarRange();
    repaintBackground();
}
PopupItems SoundModuleSection::createPopupMenu()
{
    PopupItems options;
    options.addItem(1, "add osc" );
    options.addItem(2, "add filt");
    options.addItem(3, "add string");
    return options;
}

ModuleSection* SoundModuleSection::createNewObject (const juce::ValueTree& v)

{
    auto parent = findParentComponentOfClass<SynthGuiInterface>();
    LEAF* leaf = parent->getLEAF();
    std::any args = std::make_tuple(parent->getSynth()->getEngine(), v,leaf );

    try {

        auto proc = factory.create(v.getProperty(IDs::type).toString().toStdString(),args);
        auto *module_section = new ModuleSection(v.getProperty(IDs::type).toString() + v.getProperty(IDs::uuid).toString() , v, (proc->createEditor()));
        container_->addSubSection(module_section);
        module_section->setInterceptsMouseClicks(false,true);
        parentHierarchyChanged();
        parent->tryEnqueueProcessorInitQueue(
            [this, proc] {
                SynthGuiInterface* _parent = findParentComponentOfClass<SynthGuiInterface>();
                _parent->addProcessor(proc, 0);
            });
        return module_section;
    } catch (const std::bad_any_cast& e) {
        std::cerr << "Error during object creation: " << e.what() << std::endl;
    }


    return nullptr;
}

void SoundModuleSection::deleteObject (ModuleSection* at)
{
    auto parent = findParentComponentOfClass<SynthGuiInterface>();
    at->destroyOpenGlComponents(*parent->getOpenGlWrapper());
    delete at;
}
std::map<std::string, SynthSlider*> SoundModuleSection::getAllSliders()
{
    return container_->getAllSliders();
}