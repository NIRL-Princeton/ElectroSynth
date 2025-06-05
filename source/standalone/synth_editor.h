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

#include <juce_audio_utils/juce_audio_utils.h>
#include "synth_base.h"
#include "synth_gui_interface.h"

//#include <melatonin_perfetto/melatonin_perfetto.h>
//class SynthComputerKeyboard;

class SynthEditor : public AudioAppComponent, public SynthBase, public SynthGuiInterface {
  public:
    SynthEditor(bool use_gui = true);
    ~SynthEditor();

    void prepareToPlay(int buffer_size, double sample_rate) override;
    void getNextAudioBlock(const AudioSourceChannelInfo& buffer) override;
    void releaseResources() override;
    void paint(Graphics& g) override { }
    void resized() override;

    const CriticalSection& getCriticalSection() override { return critical_section_; }
    void pauseProcessing(bool pause) override {
      if (pause)
        critical_section_.enter();
      else
        critical_section_.exit();
    }
    SynthGuiInterface* getGuiInterface() override { return static_cast<SynthGuiInterface*> (this); }

    AudioDeviceManager* getAudioDeviceManager() override { return &deviceManager; }


    void animate(bool animate);

  private:
    //std::unique_ptr<SynthComputerKeyboard> computer_keyboard_;
    CriticalSection critical_section_;
    double current_time_;
#if PERFETTO
    std::unique_ptr<perfetto::TracingSession> tracingSession;
#endif
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthEditor)
};

