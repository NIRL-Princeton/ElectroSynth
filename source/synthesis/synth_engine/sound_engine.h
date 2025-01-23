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
#include <juce_audio_processors/juce_audio_processors.h>
#include "../framework/note_handler.h"
#include <leaf.h>
#include <leaf-config.h>
//#include "buffer_debugger.h"
#include "processors/processor.h"
#include "ModulationConnection.h"
class MappingWrapper;
class ProcessorBase;
class ModulatorBase;
namespace electrosynth {
    using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = juce::AudioProcessorGraph::Node;
  class SoundEngine : public NoteHandler {
    public:
      static constexpr int kDefaultOversamplingAmount = 2;
      static constexpr int kDefaultSampleRate = 44100;

      SoundEngine();
      virtual ~SoundEngine();

//      void init() override;
      void processWithInput(const  float* audio_in, int num_samples) {
          //_ASSERT(num_samples <= output()->buffer_size);

          juce::FloatVectorOperations::disableDenormalisedNumberSupport();






      }
      void process(juce::AudioSampleBuffer&, juce::MidiBuffer &);
      void processMappings();
      void releaseResources()
      {}

      void prepareToPlay(double sampleRate, int samplesPerBlock)
      {
          setSampleRate(sampleRate);
          setBufferSize(samplesPerBlock);

      }

      //void correctToTime(double seconds) override;
      int getDefaultSampleRate()
      {
          return kDefaultSampleRate;
      }

      int getSampleRate()
      {
          return curr_sample_rate;
      }

      void setSampleRate(int sampleRate)
      {
          curr_sample_rate = sampleRate;
      }

      void setBufferSize(int bufferSize)
      {
          buffer_size = bufferSize;
      }

      int getBufferSize()
      {
          return buffer_size;
      }

      int getNumPressedNotes();

      int getNumActiveVoices();

      float getLastActiveNote() const;

      void allSoundsOff() override;
      void allNotesOff(int sample) override;
      void allNotesOff(int sample, int channel) override;
      void allNotesOffRange(int sample, int from_channel, int to_channel);

      void noteOn(int note, float velocity, int sample, int channel) override;
      void noteOff(int note, float lift, int sample, int channel) override;
      void setModWheel(float value, int channel);
      void setModWheelAllChannels(float value);
      void setPitchWheel(float value, int channel);
      void setZonedPitchWheel(float value, int from_channel, int to_channel);


      //void setBpm(float bpm);
      void setAftertouch(float note, float value, int sample, int channel);
      void setChannelAftertouch(int channel, float value, int sample);
      void setChannelRangeAftertouch(int from_channel, int to_channel, float value, int sample);
      void setChannelSlide(int channel, float value, int sample);
      void setChannelRangeSlide(int from_channel, int to_channel, float value, int sample);

      void sustainOn(int channel);
      void sustainOff(int sample, int channel);
      void sostenutoOn(int channel);
      void sostenutoOff(int sample, int channel);

      void sustainOnRange(int from_channel, int to_channel);
      void sustainOffRange(int sample, int from_channel, int to_channel);
      void sostenutoOnRange(int from_channel, int to_channel);
      void sostenutoOffRange(int sample, int from_channel, int to_channel);
      force_inline int getOversamplingAmount() const { return last_oversampling_amount_; }


      ModulationConnectionBank modulation_bank_;
      ModulationConnectionBank& getModulationBank() { return modulation_bank_; }
      void checkOversampling();

      leaf::Processor* getLEAFProcessor(const std::string&);
      leaf::Processor* getLEAFProcessorModulator(const std::string&);
      std::pair<leaf::Processor*, int> getParameterInfo(const std::string&);
      std::vector<std::vector<std::shared_ptr<ProcessorBase>>> processors;
      std::vector<std::vector<std::shared_ptr<ModulatorBase>>> modSources;
      std::vector<MappingWrapper*> mappings;
      void disconnectMapping(const electrosynth::mapping_change& change) {}
      void connectMapping (const electrosynth::mapping_change& change);
      ProcessorBase* getProcessorFromUUID(int uuid);
      ModulatorBase* getModulatorFromUUID(int uuid);

      leaf::tProcessor * getLeafProcessorFromUUID(int uuid);
      char dummy_memory[32];
      juce::AudioSampleBuffer bu{2,1};
      LEAF leaf;
    private:
      void setOversamplingAmount(int oversampling_amount, int sample_rate);
      int last_oversampling_amount_;
      int last_sample_rate_;
      int buffer_size;
      int curr_sample_rate;
//      Value* oversampling_;
//      Value* bps_;
//      Value* legato_;
//      Decimator* decimator_;
//      PeakMeter* peak_meter_;

//      std::unique_ptr<BufferDebugger> bufferDebugger;
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundEngine)
  };
} // namespace vital

