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

#pragma once
#include "audio_routing_manager.h"

#if HEADLESS

class FullInterface { };
class AudioDeviceManager { };

#endif
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ApplicationCommandHandler.h"
class LEAF;
class FullInterface;
class SynthBase;
struct OpenGlWrapper;
struct SynthGuiData {
  SynthGuiData(SynthBase* synth_base);
  juce::ValueTree& tree;
  juce::UndoManager& um;
  SynthBase* synth;
};
class ModulatorBase;
class ProcessorBase;
namespace electrosynth
{
    class ModulationConnection;
    class AudioConnection;
}
class SynthGuiInterface :  public juce::ApplicationCommandTarget, public AudioRoutingManager::Listener {
  public:
    SynthGuiInterface(SynthBase* synth, bool use_gui = true);
    virtual ~SynthGuiInterface();
    // Define your command IDs
    enum CommandIDs {
        undo = 0x2000,
        redo,
        save,
        load
    };

    void getAllCommands(juce::Array<juce::CommandID> &commands) override {
        commands.addArray({undo, redo});
    }

    void getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo &info) override {
        switch (id) {
            case undo:
                info.setInfo("Undo", "Undo last action", "Edit", 0);
            info.addDefaultKeypress('z', juce::ModifierKeys::commandModifier);
            break;
            case redo:
                info.setInfo("Redo", "Redo last action", "Edit", 0);
            info.addDefaultKeypress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        }
    }

    bool perform(const InvocationInfo &info) override;
    ApplicationCommandTarget* getNextCommandTarget() override {return nullptr;}

    virtual juce::AudioDeviceManager* getAudioDeviceManager() { return nullptr; }
    SynthBase* getSynth() { return synth_; }
    virtual void updateFullGui();
  juce::File getActiveFile();
   void openLoadDialog();

    void audioConnectionCreated(const electrosynth::audio::AudioConnection& connection) override;
    void audioConnectionRemoved(const electrosynth::audio::AudioConnection& connection) override;

    virtual void updateGuiControl(const std::string& name, float value);
    void tryEnqueueProcessorInitQueue(juce::FixedSizeFunction<48, void()> callback);
    void addProcessor(std::unique_ptr<ProcessorBase> processor, int voice_index);
    void addModulationSource(std::unique_ptr<ModulatorBase> modSource, int voice_index);
    bool connectModulation(std::string source, std::string destination, int destination_slot = -1);
    void disconnectModulation(std::string source, std::string destination);
    void disconnectModulation(electrosynth::ModulationConnection* connection);
    void notifyModulationsChanged();
    void setFocus();
    void notifyChange();
    void notifyFresh();
    void openSaveDialog();
    void externalPresetLoaded(juce::File preset);
    void setGuiSize(float scale);
  bool loadFromFile(juce::File preset, std::string &error);

    FullInterface* getGui() { return gui_.get(); }
    LEAF* getLEAF();
    OpenGlWrapper* getOpenGlWrapper();
  std::unique_ptr<ApplicationCommandHandler> commandHandler;
  juce::ApplicationCommandManager commandManager;
  protected:
  std::atomic<bool> loading;
    SynthBase* synth_;
  std::unique_ptr<juce::FileChooser> filechooser;
    std::unique_ptr<FullInterface> gui_;


  
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthGuiInterface)
};
