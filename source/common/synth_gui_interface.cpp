/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synth_gui_interface.h"

#include "../synthesis/synth_engine/sound_engine.h"
#include "Modulators/EnvModuleProcessor.h"
#include "Modulators/ModulatorBase.h"
#include "Processors/ProcessorBase.h"
#include "load_save.h"
#include "synth_base.h"

SynthGuiData::SynthGuiData(SynthBase* synth_base) : synth(synth_base),
                                                     tree(synth_base->tree),
                                                     um(synth_base->getUndoManager())
{

}
#if HEADLESS

SynthGuiInterface::SynthGuiInterface(SynthBase* synth, bool use_gui) : synth_(synth) { }
SynthGuiInterface::~SynthGuiInterface() { }
void SynthGuiInterface::updateFullGui() { }
void SynthGuiInterface::updateGuiControl(const std::string& name, float value) { }
float SynthGuiInterface::getControlValue(const std::string& name) { return 0.0f; }
void SynthGuiInterface::connectModulation(std::string source, std::string destination) { }
void SynthGuiInterface::connectModulation(electrosynth::ModulationConnection* connection) { }
void SynthGuiInterface::setModulationValues(const std::string& source, const std::string& destination,
                                            float amount, bool bipolar, bool stereo, bool bypass) { }
void SynthGuiInterface::disconnectModulation(std::string source, std::string destination) { }
void SynthGuiInterface::disconnectModulation(electrosynth::ModulationConnection* connection) { }
void SynthGuiInterface::setFocus() { }
void SynthGuiInterface::notifyChange() { }
void SynthGuiInterface::notifyFresh() { }
void SynthGuiInterface::openSaveDialog() { }
void SynthGuiInterface::externalPresetLoaded(File preset) { }
void SynthGuiInterface::setGuiSize(float scale) { }

#else

    #include "synth_base.h"
#include <memory>
#include "../interface/look_and_feel/default_look_and_feel.h"
#include "../interface/fullInterface.h"


SynthGuiInterface::SynthGuiInterface(SynthBase* synth, bool use_gui) : synth_(synth) {
  if (use_gui) {
    SynthGuiData synth_data(synth_);
    gui_ = std::make_unique<FullInterface>(&synth_data);
    // for registering hotkeys etc.
    commandHandler = std::make_unique<ApplicationCommandHandler>(this);
    commandManager.registerAllCommandsForTarget(commandHandler.get());
  }


}

SynthGuiInterface::~SynthGuiInterface() { }

bool SynthGuiInterface::perform(const InvocationInfo & info) {
    {
        switch (info.commandID) {
            case undo:
            {
                synth_->um.undo();
                // juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Undo", " Undo triggered");
                return true;
            }
            case redo:
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Redo", "Redo triggered");
                return true;
            }
            default:
                return false;
        }
    }
}

void SynthGuiInterface::updateFullGui() {
  if (gui_ == nullptr)
    return;

//  gui_->setAllValues(synth_->getControls());
  gui_->reset();
}
juce::File SynthGuiInterface::getActiveFile() {
    return synth_->getActiveFile();
}


void SynthGuiInterface::openLoadDialog() {
    auto active_file = getActiveFile();
    filechooser = std::make_unique<juce::FileChooser>("Open Preset", active_file,
                                                      juce::String("*.") + electrosynth::kPresetExtension);

    auto flags = juce::FileBrowserComponent::openMode
                 | juce::FileBrowserComponent::canSelectFiles;
    filechooser->launchAsync(flags, [this](const juce::FileChooser &fc) {
        if (fc.getResult() == juce::File{}) {
            return;
        }

        std::string error;
        juce::File choice = fc.getResult();
        loading = true;
        if (!this->loadFromFile(choice, error)) {
            //            std::string name = ProjectInfo::projectName;
            //            error = "There was an error open the preset. " + error;
            //juce::AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "PRESET ERROR, ""Error opening preset", error);
            DBG(error);
        }
        loading = false;
        //        else
        //            parent->externalPresetLoaded(choice);
        DBG("==============filescho====================");
    });
}
bool SynthGuiInterface::loadFromFile(juce::File preset, std::string &error) {
    return getSynth()->loadFromFile(preset, error);
    //sampleLoadManager->loadSamples()
}

void SynthGuiInterface::openSaveDialog() {
    filechooser = std::make_unique<juce::FileChooser>("Export the gallery", juce::File(),
                                                      juce::String("*.") + electrosynth::kPresetExtension, true);
    filechooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
        juce::FileBrowserComponent::canSelectDirectories,
        [this](const juce::FileChooser &chooser) {


            auto result = chooser.getURLResult();
            auto name = result.isEmpty()
                            ? juce::String()
                            : (result.isLocalFile()
                                   ? result.getLocalFile().getFullPathName()
                                   : result.toString(true));
            juce::File file(name);

            //savetofile(file)
            for (auto outerVT : getSynth()->tree)
            {
                //get all IDs::CHAIN
                if (outerVT.hasType (IDs::CHAINS))
                    for (auto innerVT : outerVT)
                    {
                        if (innerVT.hasType (IDs::CHAIN))
                            innerVT.setProperty(IDs::sync, 1, nullptr);
                    }
                //get all IDs::MODULATORS
                if (outerVT.hasType (IDs::MODULATORS))
                    outerVT.setProperty(IDs::sync, 1, nullptr);
                //get All IDs::EFFECT
                if (outerVT.hasType (IDs::EFFECTS))
                    outerVT.setProperty(IDs::sync, 1, nullptr);
            }

            //get MASTER VOICE ENVELOPE IDs::MODULATOR (maybe we want to change this to  have a special ID)
            juce::MemoryBlock data;
            auto & obj = getSynth()->getEngine()->MasterVoiceEnvelopeProcessor;
                obj->getStateInformation(data);
                auto myxml = juce::parseXML(data.toString());
                //auto xml = juce::AudioProcessor::getXmlF(data.getData(), (int)data.getSize());
                if (obj->state.isValid() && myxml != nullptr) {
                    auto uuid = obj->state.getProperty(IDs::uuid).toString();
                    auto type = obj->state.getProperty(IDs::type).toString();
                    obj->state.copyPropertiesFrom(juce::ValueTree::fromXml(*myxml),nullptr);
                    obj->state.setProperty(IDs::type, type,nullptr);
                    obj->state.setProperty(IDs::uuid, uuid,nullptr);
                    //  state.addChild(juce::ValueTree::fromXml(*xml),0,nullptr);
                }
            // getSynth()->tree.getChildWithName(IDs::CHAINS).getChildWithName(IDs::CHAIN).setProperty(IDs::sync, 1, nullptr);

            juce::String mystr = (getSynth()->tree.toXmlString());
            auto xml = getSynth()->tree.createXml();
            juce::XmlElement xml_ = *xml;
            if (!result.isEmpty()) {
                juce::FileOutputStream output(file);
                output.setPosition(0);
                output.truncate();
                output.writeText(xml_.toString(), false, false, {});
                //                                         std::unique_ptr<juce::InputStream> wi (file.createInputStream());
                //                                         std::unique_ptr<juce::OutputStream> wo (result.createOutputStream());
                //
                //                                         if (wi != nullptr && wo != nullptr)
                //                                         {
                //                                             //auto numWritten = wo->writeFromInputStream (*wi, -1);
                //                                             wo->flush();
                //                                         }
                output.flush();
            }
            //savetofile
        });
}


void SynthGuiInterface::updateGuiControl(const std::string& name, float value) {
  if (gui_ == nullptr)
    return;

//  gui_->setValue(name, value, NotificationType::dontSendNotification);
}

LEAF* SynthGuiInterface::getLEAF()
{return synth_->getLeaf();}
void SynthGuiInterface::tryEnqueueProcessorInitQueue (juce::FixedSizeFunction<48, void()> callback) {
  synth_->processorInitQueue.try_enqueue(std::move(callback));
}
void SynthGuiInterface::addProcessor(std::unique_ptr<ProcessorBase> processor, int voice_index)
{
  synth_->addProcessor (std::move(processor), voice_index);
}

void SynthGuiInterface::addModulationSource (std::unique_ptr<ModulatorBase> modSource, int voice_index)
{
    synth_->addModulationSource(std::move(modSource),voice_index);
}
void SynthGuiInterface::connectModulation(std::string source, std::string destination)
{
    bool created = synth_->connectModulation(source, destination);
//    if (created)
//        return;
        //initModulationValues(source, destination);
    notifyModulationsChanged();
}

void SynthGuiInterface::disconnectModulation(std::string source, std::string destination) {
    synth_->disconnectModulation(source, destination);
    notifyModulationsChanged();
}

void SynthGuiInterface::disconnectModulation(electrosynth::ModulationConnection* connection) {
    synth_->disconnectModulation(connection);
    notifyModulationsChanged();
}
void SynthGuiInterface::notifyModulationsChanged() {
    gui_->modulationChanged();
}
//float SynthGuiInterface::getControlValue(const std::string& name) {
//  return synth_->getControls()[name]->value();
//}

//void SynthGuiInterface::notifyModulationsChanged() {
//  gui_->modulationChanged();
//}

//void SynthGuiInterface::notifyModulationValueChanged(int index) {
//  gui_->modulationValueChanged(index);
//}
//
//void SynthGuiInterface::connectModulation(std::string source, std::string destination) {
//  bool created = synth_->connectModulation(source, destination);
//  if (created)
//    initModulationValues(source, destination);
//  notifyModulationsChanged();
//}

//void SynthGuiInterface::connectModulation(electrosynth::ModulationConnection* connection) {
//  synth_->connectModulation(connection);
//  notifyModulationsChanged();
//}

OpenGlWrapper* SynthGuiInterface::getOpenGlWrapper() {
  return &gui_->open_gl_;
}





void SynthGuiInterface::setFocus() {
  if (gui_ == nullptr)
    return;

  gui_->setFocus();
}

void SynthGuiInterface::notifyChange() {
  if (gui_ == nullptr)
    return;

  gui_->notifyChange();
}

void SynthGuiInterface::notifyFresh() {
  if (gui_ == nullptr)
    return;

  gui_->notifyFresh();
}




void SynthGuiInterface::setGuiSize(float scale) {
  if (gui_ == nullptr)
    return;

  juce::Point<int> position = gui_->getScreenBounds().getCentre();
  const Displays::Display& display = Desktop::getInstance().getDisplays().findDisplayForPoint(position);

  Rectangle<int> display_area = Desktop::getInstance().getDisplays().getTotalBounds(true);
  ComponentPeer* peer = gui_->getPeer();
  if (peer)
    peer->getFrameSize().subtractFrom(display_area);

  float window_size = scale / display.scale;
  window_size = std::min(window_size, display_area.getWidth() * 1.0f / electrosynth::kDefaultWindowWidth);
  window_size = std::min(window_size, display_area.getHeight() * 1.0f / electrosynth::kDefaultWindowHeight);
  //LoadSave::saveWindowSize(window_size);

  int width = std::round(window_size * electrosynth::kDefaultWindowWidth);
  int height = std::round(window_size * electrosynth::kDefaultWindowHeight);

  Rectangle<int> bounds = gui_->getBounds();
  bounds.setWidth(width);
  bounds.setHeight(height);
  gui_->getParentComponent()->setBounds(bounds);
  gui_->redoBackground();
}
#endif
