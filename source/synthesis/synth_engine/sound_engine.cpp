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

#include "sound_engine.h"
#include "OscillatorModuleProcessor.h"
#include "melatonin_audio_sparklines/melatonin_audio_sparklines.h"
#include "Modulators/ModulatorBase.h"
#include "ModulationWrapper.h"
#include "Processors/ProcessorBase.h"
namespace electrosynth {

  SoundEngine::SoundEngine() : /*voice_handler_(nullptr),*/
                                last_oversampling_amount_(-1), last_sample_rate_(-1), bufferDebugger(std::make_unique<BufferDebugger>()), modulation_bank_((leaf))
  {
      LEAF_init(&leaf, 44100.0f, dummy_memory, 32, [](){return (float)rand()/RAND_MAX;});
      //processors.push_back(std::make_shared<OscillatorModuleProcessor> (&leaf));
    //SoundEngine::init();
  }

  SoundEngine::~SoundEngine() {
    //voice_handler_->prepareDestroy();
  }

//  void SoundEngine::init() {
//
//
//
//gg
//    //SynthModule::init();
//
//    setOversamplingAmount(kDefaultOversamplingAmount, kDefaultSampleRate);
//  }



  int SoundEngine::getNumPressedNotes() {
    //return voice_handler_->getNumPressedNotes();
  }



  int SoundEngine::getNumActiveVoices() {
    //return voice_handler_->getNumActiveVoices();
  }



  float SoundEngine::getLastActiveNote() const {
    //return voice_handler_->getLastActiveNote();
  }



  void SoundEngine::checkOversampling() {
    //int oversampling = oversampling_->value();
   // int oversampling_amount = 1 << oversampling;
    //int sample_rate = getSampleRate();
//    if (last_oversampling_amount_ != oversampling_amount || last_sample_rate_ != sample_rate)
//      setOversamplingAmount(oversampling_amount, sample_rate);
  }

  void SoundEngine::setOversamplingAmount(int oversampling_amount, int sample_rate) {
    static constexpr int kBaseSampleRate = 44100;

    int oversample = oversampling_amount;
    int sample_rate_mult = sample_rate / kBaseSampleRate;
    while (sample_rate_mult > 1 && oversample > 1) {
      sample_rate_mult >>= 1;
      oversample >>= 1;
    }
    //voice_handler_->setOversampleAmount(oversample);

    last_oversampling_amount_ = oversampling_amount;
    last_sample_rate_ = sample_rate;
  }

  void SoundEngine::process(juce::AudioSampleBuffer &audio_buffer, juce::MidiBuffer& midi_buffer )
  {
    //VITAL_ASSERT(num_samples <= output()->buffer_size);
    juce::FloatVectorOperations::disableDenormalisedNumberSupport();
    audio_buffer.clear();
    bu.clear();
    //juce::MidiBuffer midimessages;
    for (int i = 0; i < audio_buffer.getNumSamples(); i++){
      for (auto proc_chain : processors)
      {
          for (auto proc : proc_chain)
          {
              proc->processBlock (bu, midi_buffer);

          }
          audio_buffer.addSample(0,i,bu.getSample(0,0));
          audio_buffer.addSample(1,i,bu.getSample(1,0));
          bu.clear();
      }

  }
  //melatonin::printSparkline (audio_buffer);
    if (getNumActiveVoices() == 0)
    {

    }
   bufferDebugger->capture("main out", audio_buffer.getReadPointer(0), audio_buffer.getNumSamples(), -20.f, 20.f);
  }



//  void SoundEngine::correctToTime(double seconds) {
////    voice_handler_->correctToTime(seconds);
////    effect_chain_->correctToTime(seconds);
//  }

  void SoundEngine::allSoundsOff() {
//    voice_handler_->allSoundsOff();
//    effect_chain_->hardReset();
//    decimator_->hardReset();
  }

  void SoundEngine::allNotesOff(int sample) {
//    voice_handler_->allNotesOff(sample);
  }

  void SoundEngine::allNotesOff(int sample, int channel) {
//    voice_handler_->allNotesOff(channel);
  }

  void SoundEngine::allNotesOffRange(int sample, int from_channel, int to_channel) {
//    voice_handler_->allNotesOffRange(sample, from_channel, to_channel);
  }

  void SoundEngine::noteOn(int note, float velocity, int sample, int channel) {
//    voice_handler_->noteOn(note, velocity, sample, channel);
  }

  void SoundEngine::noteOff(int note, float lift, int sample, int channel) {
//    voice_handler_->noteOff(note, lift, sample, channel);
  }

  void SoundEngine::setModWheel(float value, int channel) {
//    voice_handler_->setModWheel(value, channel);
  }

  void SoundEngine::setModWheelAllChannels(float value) {
//    voice_handler_->setModWheelAllChannels(value);
  }
  
  void SoundEngine::setPitchWheel(float value, int channel) {
//    voice_handler_->setPitchWheel(value, channel);
  }

  void SoundEngine::setZonedPitchWheel(float value, int from_channel, int to_channel) {
//    voice_handler_->setZonedPitchWheel(value, from_channel, to_channel);
  }


  void SoundEngine::setAftertouch(float note, float value, int sample, int channel) {
//    voice_handler_->setAftertouch(note, value, sample, channel);
  }

  void SoundEngine::setChannelAftertouch(int channel, float value, int sample) {
//    voice_handler_->setChannelAftertouch(channel, value, sample);
  }

  void SoundEngine::setChannelRangeAftertouch(int from_channel, int to_channel, float value, int sample) {
//    voice_handler_->setChannelRangeAftertouch(from_channel, to_channel, value, sample);
  }

  void SoundEngine::setChannelSlide(int channel, float value, int sample) {
//    voice_handler_->setChannelSlide(channel, value, sample);
  }

  void SoundEngine::setChannelRangeSlide(int from_channel, int to_channel, float value, int sample) {
//    voice_handler_->setChannelRangeSlide(from_channel, to_channel, value, sample);
  }

//  void SoundEngine::setBpm(float bpm) {
//    float bps = bpm / 60.0f;
//    if (bps_->value() != bps)
//      bps_->set(bps);
//  }


//  Sample* SoundEngine::getSample() {
//    return voice_handler_->getSample();
//  }



  void SoundEngine::sustainOn(int channel) {
//    voice_handler_->sustainOn(channel);
  }

  void SoundEngine::sustainOff(int sample, int channel) {
//    voice_handler_->sustainOff(sample, channel);
  }

  void SoundEngine::sostenutoOn(int channel) {
//    voice_handler_->sostenutoOn(channel);
  }

  void SoundEngine::sostenutoOff(int sample, int channel) {
//    voice_handler_->sostenutoOff(sample, channel);
  }

  void SoundEngine::sustainOnRange(int from_channel, int to_channel) {
//    voice_handler_->sustainOnRange(from_channel, to_channel);
  }

  void SoundEngine::sustainOffRange(int sample, int from_channel, int to_channel) {
//    voice_handler_->sustainOffRange(sample, from_channel, to_channel);
  }

  void SoundEngine::sostenutoOnRange(int from_channel, int to_channel) {
//    voice_handler_->sostenutoOnRange(from_channel, to_channel);
  }

  void SoundEngine::sostenutoOffRange(int sample, int from_channel, int to_channel) {
//    voice_handler_->sostenutoOffRange(sample, from_channel, to_channel);
  }

  leaf::Processor* SoundEngine::getLEAFProcessor (const std::string& proc_string) {

      // Use find_if to search the outermost vector
      auto outerIt = std::find_if(processors.begin(), processors.end(), [&](const std::vector<std::shared_ptr<ProcessorBase>>& innerVec) {
          // Use find_if on the inner vector to look for the processor with the target name
          auto innerIt = std::find_if(innerVec.begin(), innerVec.end(), [&](const std::shared_ptr<ProcessorBase>& processor) {
              return processor->name ==  juce::String(proc_string);
          });

          // Return true if the processor was found in this inner vector
          return innerIt != innerVec.end();
      });

      if (outerIt != processors.end()) {
          auto innerIt = std::find_if(outerIt->begin(), outerIt->end(),
              [&](const std::shared_ptr<ProcessorBase>& processor) {
                  return processor->name == juce::String(proc_string);
              });

          // Here you can cast the processor to leaf::Processor* if needed
          return &(innerIt->get()->proc);
      }

  }
  std::pair<leaf::Processor*, int> SoundEngine::getParameterInfo (const std::string& value) {
      std::stringstream ss(value);
      std::string proc_string;
      std::getline(ss,proc_string,'_');

      auto proc = getLEAFProcessor(proc_string);

      std::string param_string;
      std::getline(ss,param_string,'_');


  }


} // namespace vital
