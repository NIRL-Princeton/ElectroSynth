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


#if HEADLESS

class FullInterface { };
class AudioDeviceManager { };

#endif
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

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
}
class SynthGuiInterface {
  public:
    SynthGuiInterface(SynthBase* synth, bool use_gui = true);
    virtual ~SynthGuiInterface();

    virtual juce::AudioDeviceManager* getAudioDeviceManager() { return nullptr; }
    SynthBase* getSynth() { return synth_; }
    virtual void updateFullGui();
    virtual void updateGuiControl(const std::string& name, float value);
    void tryEnqueueProcessorInitQueue(juce::FixedSizeFunction<48, void()> callback);
    void addProcessor(std::unique_ptr<ProcessorBase> processor, int voice_index);
    void addModulationSource(std::unique_ptr<ModulatorBase> modSource, int voice_index);
    void connectModulation(std::string source, std::string destination);
    void disconnectModulation(std::string source, std::string destination);
    void disconnectModulation(electrosynth::ModulationConnection* connection);
    void notifyModulationsChanged();
    void setFocus();
    void notifyChange();
    void notifyFresh();
    void openSaveDialog();
    void externalPresetLoaded(juce::File preset);
    void setGuiSize(float scale);
    FullInterface* getGui() { return gui_.get(); }
    LEAF* getLEAF();
    OpenGlWrapper* getOpenGlWrapper();
  protected:
    SynthBase* synth_;

    std::unique_ptr<FullInterface> gui_;
  
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthGuiInterface)
};

