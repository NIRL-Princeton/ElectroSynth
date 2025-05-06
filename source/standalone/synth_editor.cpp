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





#include "synth_editor.h"

#include "default_look_and_feel.h"
#include "FullInterface.h"
#include "../synthesis/synth_engine/sound_engine.h"

#include "startup.h"
#include "utils.h"
#include <memory>


SynthEditor::SynthEditor(bool use_gui) : SynthGuiInterface(this, use_gui) {
  static constexpr int kHeightBuffer = 50;
#if PERFETTO
  MelatoninPerfetto::get().beginSession();
#endif
  //computer_keyboard_ = std::make_unique<SynthComputerKeyboard>(engine_.get(), keyboard_state_.get());
  current_time_ = 0.0;

  setAudioChannels(0, electrosynth::kNumChannels);

  AudioDeviceManager::AudioDeviceSetup setup;
  deviceManager.getAudioDeviceSetup(setup);
  setup.sampleRate = electrosynth::kDefaultSampleRate;
  deviceManager.initialise(0, electrosynth::kNumChannels, nullptr, true, "", &setup);

  if (deviceManager.getCurrentAudioDevice() == nullptr) {
    const OwnedArray<AudioIODeviceType>& device_types = deviceManager.getAvailableDeviceTypes();

    for (AudioIODeviceType* device_type : device_types) {
      deviceManager.setCurrentAudioDeviceType(device_type->getTypeName(), true);
      if (deviceManager.getCurrentAudioDevice())
        break;
    }
  }

  current_midi_ins_ = StringArray(juce::MidiInput::getDevices());

  for (const String& midi_in : current_midi_ins_)
    deviceManager.setMidiInputDeviceEnabled(midi_in, true);

  deviceManager.addMidiInputDeviceCallback("", static_cast<juce::MidiInputCallback*>(midi_manager_.get()));

  if (use_gui) {
    setLookAndFeel(DefaultLookAndFeel::instance());
    addAndMakeVisible(gui_.get());
    gui_->reset();


    Rectangle<int> total_bounds = Desktop::getInstance().getDisplays().getTotalBounds(true);
    total_bounds.removeFromBottom(kHeightBuffer);

    float window_size = 1.0f;
    window_size = std::min(window_size, total_bounds.getWidth() / (1.0f * electrosynth::kDefaultWindowWidth));
    window_size = std::min(window_size, total_bounds.getHeight() / (1.0f * electrosynth::kDefaultWindowHeight));
    int width = std::round(window_size * electrosynth::kDefaultWindowWidth);
    int height = std::round(window_size * electrosynth::kDefaultWindowHeight);
    setSize(width, height);

    setWantsKeyboardFocus(true);
    //addKeyListener(computer_keyboard_.get());
    setOpaque(true);
  }

  startTimer(500);
}

SynthEditor::~SynthEditor() {
#if PERFETTO
  MelatoninPerfetto::get().endSession();
#endif
  PopupMenu::dismissAllActiveMenus();
  shutdownAudio();
}

void SynthEditor::prepareToPlay(int buffer_size, double sample_rate) {
  //engine_->setSampleRate(sample_rate);
  engine_->prepareToPlay(sample_rate, buffer_size);
  midi_manager_->setSampleRate(sample_rate);
}

void SynthEditor::getNextAudioBlock(const AudioSourceChannelInfo& buffer) {
  ScopedLock lock(getCriticalSection());

  int num_samples = buffer.buffer->getNumSamples();
  int synth_samples = std::min(num_samples, electrosynth::kMaxBufferSize);

  //processModulationChanges();
  MidiBuffer midi_messages;
  midi_manager_->removeNextBlockOfMessages(midi_messages, num_samples);
  processKeyboardEvents(midi_messages, num_samples);

  double sample_time = 1.0 / getSampleRate();
  for (int b = 0; b < num_samples; b += synth_samples) {
    int current_samples = std::min<int>(synth_samples, num_samples - b);
    //engine_->correctToTime(current_time_);

    processMidi(midi_messages, b, b + current_samples);
    processAudio(buffer.buffer, electrosynth::kNumChannels, current_samples, b);
    current_time_ += current_samples * sample_time;
  }
    //processAudioAndMidi(*buffer.buffer, midi_messages);
}

void SynthEditor::releaseResources() {
    engine_->releaseResources();
}

void SynthEditor::resized() {
  if (gui_)
  {


                  const auto scaleFactor = static_cast<float> (getWidth()) / electrosynth::kDefaultWindowWidth;
                  gui_->setTransform (AffineTransform::scale (scaleFactor));
                  gui_->setBounds(0, 0, electrosynth::kDefaultWindowWidth, electrosynth::kDefaultWindowHeight);

    //gui_->setBounds (getBounds());
  }
}

void SynthEditor::timerCallback() {
  StringArray midi_ins(MidiInput::getDevices());

  for (int i = 0; i < midi_ins.size(); ++i) {
    if (!current_midi_ins_.contains(midi_ins[i]))
      deviceManager.setMidiInputEnabled(midi_ins[i], true);
  }

  current_midi_ins_ = midi_ins;
}

void SynthEditor::animate(bool animate) {
  if (gui_)
    gui_->animate(animate);
}
