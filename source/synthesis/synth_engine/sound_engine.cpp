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

#include "MasterVoiceProcessor.h"
#include "../framework/Processors/OscillatorModuleProcessor.h"
#include "melatonin_audio_sparklines/melatonin_audio_sparklines.h"
#include "Modulators/ModulatorBase.h"
#include "ModulationWrapper.h"
#include "Processors/ProcessorBase.h"
#include "parameterArrays.h"
#include "ModulationConnection.h"
#include "Modulators/EnvModuleProcessor.h"

namespace electrosynth {

  SoundEngine::SoundEngine() : /*voice_handler_(nullptr),*/
    last_oversampling_amount_(-1), last_sample_rate_(-1), modulation_bank_((leaf))
  {
      LEAF_init(&leaf, 44100.0f, memory, 16777216, [](){return (float)rand()/RAND_MAX;});
      //processors.push_back(std::make_shared<OscillatorModuleProcessor> (&leaf));
    //SoundEngine::init();
      tSimplePoly_init(&voiceHandler.voices[0], MAX_NUM_VOICES, &leaf);
      voiceHandler.numVoicesActive = MAX_NUM_VOICES;
      tSimplePoly_setNumVoices(voiceHandler.voices[0], (uint8_t)voiceHandler.numVoicesActive);
       voiceHandler.voiceNote[0] = 0;
       for (uint8_t i = 1; i < MAX_NUM_VOICES; i++)
       {
            tSimplePoly_init(&voiceHandler.voices[i], MAX_NUM_VOICES, &leaf);
           voiceHandler.voices[i] = 0;
           voiceHandler.voiceIsSounding[i] = false;
           voiceHandler.voicePrevBend[i] = 0.0f;
       }
      MasterVoiceEnvelopeProcessor  = std::make_unique<EnvModuleProcessor>(this, juce::ValueTree (IDs::MODULATOR).setProperty(IDs::type, "env", nullptr),&leaf);
      MasterVoiceEnvelopeProcessor->state.params.attackParam->setParameterValue(0.1);
      MasterVoiceEnvelopeProcessor->state.params.decayParam->setParameterValue(0.01);
      MasterVoiceEnvelopeProcessor->state.params.releaseParam->setParameterValue(0.001);
      //temp_voice_buffer.set
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

  void SoundEngine::processMappings()
  {
      for (auto mapping : mappings)
      {
          processMapping(&mapping->mapping_);
      }
  }
    void SoundEngine::process(juce::AudioSampleBuffer &audio_buffer , int channels, int samples, int offset)
  {
      //VITAL_ASSERT(num_samples <= output()->buffer_size);
      // benchmark();
      juce::FloatVectorOperations::disableDenormalisedNumberSupport();
      temp_voice_buffer.clear();
      //juce::MidiBuffer midimessages;
      int mpe = voiceHandler.mpeMode ? 1 : 0;
      int impe = 1-mpe;
      for (int i = offset; i < samples + offset; i++){
          for (int v = 0; v < voiceHandler.numVoicesActive; ++v)
        {

            float tempNote = (float)tSimplePoly_getPitch(voiceHandler.voices[v*mpe],(uint8_t) (v*impe));


            //added this check because if there is no active voice "getPitch" returns -1
            if (tempNote >= 0.0f)
            {
                //freeze pitch bend data on voices where a note off has happened and we are in the release phase
                if (tSimplePoly_isOn(voiceHandler.voices[v*mpe], (uint8_t)(v*impe)))
                {
                    //tempNote += pitchBend;
                   // voicePrevBend[v] = pitchBend;
                }
                else
                {
                    //tempNote += voicePrevBend[v];
                }
                if ((tempNote >= 0.0f) && (tempNote < 127.0f))
                {
                    int tempNoteIntPart = (int)tempNote;
                    float tempNoteFloatPart = tempNote - (float)tempNoteIntPart;
                    //int tempPitchClassIntPart =tempNoteIntPart % 12;
                    //float dev1 = (centsDeviation[tempNoteIntPart] * (1.0f - tempNoteFloatPart));
                    //float dev2 =  (centsDeviation[(tempNoteIntPart+1)] * tempNoteFloatPart);
                    //float tunedNote = ( dev1  + dev2);
                    voiceHandler.voiceNote[v] = tempNote;
                }
                else //otherwise, assume octave equivalency and get offsets, then get midinote back
                    //not going to work for non-octave tunings
                {
                    if (!isnan(tempNote) && !isinf(tempNote))
                    {
                        // int octaveUp = 0;
                        // int octaveDown = 0;
                        //
                        //
                        // while(tempNote >= 127.0f)
                        // {
                        //     tempNote -= 12.0f;
                        //     octaveDown++;
                        // }
                        // while(tempNote < 0.0f)
                        // {
                        //     tempNote += 12.0f;
                        //     octaveUp++;
                        // }
                        //
                        // int tempNoteIntPart = (int)tempNote;
                        // float tempNoteFloatPart = tempNote - (float)tempNoteIntPart;
                        // float dev1 = (centsDeviation[tempNoteIntPart] * (1.0f - tempNoteFloatPart));
                        // float dev2 =  (centsDeviation[(tempNoteIntPart+1)] * tempNoteFloatPart);
                        // float tunedNote = ( dev1  + dev2);
                        // voiceNote[v] = tunedNote + (octaveDown*12) - (octaveUp*12);
                        voiceHandler.voiceNote[v] = tempNote;
                    }
                }
                //DBG("Tuned note" + String(tunedNote));
            }
            //samples[0][v] = 0.f;
            //samples[1][v] = 0.f;
        }
          auto amp_vals = MasterVoiceEnvelopeProcessor->processMasterEnvelope();
          processMappings();
          for (auto modulator_chain : modSources)
          {
              for(auto modulator : modulator_chain)
              {
                  modulator->process();
              }
          }
          for (auto proc_chain : processors)
          {

              for (auto proc : proc_chain)
              {
                  proc->processBlock (temp_voice_buffer, empty);

              }
              //at end of given processor chain
              for ( int v = 0; v < voiceHandler.numVoicesActive; ++v) {
                      // audio_buffer.addSample(0, i, temp_voice_buffer.getSample(v*2, 0));
                      // audio_buffer.addSample(1, i, temp_voice_buffer.getSample(v*2+1, 0));
                      audio_buffer.addSample(0, i, amp_vals->getSample(v*2, 0) * temp_voice_buffer.getSample(v*2, 0));
                     audio_buffer.addSample(1, i, amp_vals->getSample(v*2+1, 0) * temp_voice_buffer.getSample(v*2+1, 0));
              }

          }
          temp_voice_buffer.clear();



      }
      //melatonin::printSparkline (audio_buffer,false);
      if (getNumActiveVoices() == 0)
      {

      }
      //   bufferDebugger->capture("main out", audio_buffer.getReadPointer(0), audio_buffer.getNumSamples(), -20.f, 20.f);
  }
  void SoundEngine::process(juce::AudioSampleBuffer &audio_buffer, juce::MidiBuffer& midi_buffer )
  {

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
      enum MidiMainType {
          kNoteOff = 0x80,
          kNoteOn = 0x90,
          kAftertouch = 0xa0,
          kController = 0xb0,
          kProgramChange = 0xc0,
          kChannelPressure = 0xd0,
          kPitchWheel = 0xe0,
      };
      int i = voiceHandler.mpeMode ? channel : 0;
      if (i < 0) return;
      if (!velocity) noteOff(note, velocity, sample, channel);
      else
      {
          int v = tSimplePoly_noteOn(voiceHandler.voices[i], note, velocity * 127.f);
          if (!voiceHandler.mpeMode) i = v;
          DBG("note on" + String(i) + " " + String(v));

          if (v >= 0)
          {
              velocity = ((0.007685533519034f*velocity*127.f) + 0.0239372430f);
              velocity = velocity * velocity;
              //note -= midiKeyMin;
              for (int j = 0; j < voiceHandler.eventEmitter.numListeners; j++) {
                  voiceHandler.eventEmitter.listeners[v][j].setterFunctions[EVENT_WATCH_INDEX]
                                                    (voiceHandler.eventEmitter.listeners[v][j].object,velocity);
              }

              MasterVoiceEnvelopeProcessor->state.params.modules[v]->setterFunctions[EVENT_WATCH_INDEX](MasterVoiceEnvelopeProcessor->state.params.modules[v],velocity);
              voiceHandler.voiceIsSounding[v] = true;
             // float norm = key / float(mkkkidiKeyMax - midiKeyMin);

          }
      }
  }

  void SoundEngine::noteOff(int note, float velocity, int sample, int channel) {
//    voice_handler_->noteOff(note, lift, sample, channel);
      int i = voiceHandler.mpeMode ? channel : 0;

      if (i < 0) return;

      int v = tSimplePoly_markPendingNoteOff(voiceHandler.voices[i], note);

      //If stack_IsNOTEmpty
      if ((v != -1) && (tStack_getSize(voiceHandler.voices[i]->stack) >= voiceHandler.numVoicesActive)) {
          if (voiceHandler.voices[0]->voices[v][0] == -2)
          {

              tSimplePoly_deactivateVoice(voiceHandler.voices[0], v);
              voiceHandler.voiceIsSounding[v] = true;
          }
          return;
      }
      if (voiceHandler.mpeMode) v = i;
      DBG("noteoff" + String(i) + " " + String(v));

      if (v >= 0)
      {

          //note -= midiKeyMin;
          for (uint8_t j = 0; j < voiceHandler.eventEmitter.numListeners; j++) {
              voiceHandler.eventEmitter.listeners[v][j].setterFunctions[EVENT_WATCH_INDEX]
                                                (voiceHandler.eventEmitter.listeners[v][j].object,0.f);
          }

          MasterVoiceEnvelopeProcessor->state.params.modules[v]->setterFunctions[EVENT_WATCH_INDEX](MasterVoiceEnvelopeProcessor->state.params.modules[v],0.f);
          voiceHandler.voiceIsSounding[v] = true;
          // float norm = key / float(mkkkidiKeyMax - midiKeyMin);

      }
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
          return (innerIt->get()->procArray);
      }

  }

  leaf::Processor* SoundEngine::getLEAFProcessorModulator (const std::string& proc_string) {

      // Use find_if to search the outermost vector
      auto outerIt = std::find_if(modSources.begin(), modSources.end(), [&](const std::vector<std::shared_ptr<ModulatorBase>>& innerVec) {
          // Use find_if on the inner vector to look for the processor with the target name
          auto innerIt = std::find_if(innerVec.begin(), innerVec.end(), [&](const std::shared_ptr<ModulatorBase>& processor) {
              return processor->name ==  juce::String(proc_string);
          });

          // Return true if the processor was found in this inner vector
          return innerIt != innerVec.end();
      });

      if (outerIt != modSources.end()) {
          auto innerIt = std::find_if(outerIt->begin(), outerIt->end(),
              [&](const std::shared_ptr<ModulatorBase>& processor) {
                  return processor->name == juce::String(proc_string);
              });

          // Here you can cast the processor to leaf::Processor* if needed
          return (innerIt->get()->procArray);
      }

  }

  std::pair<leaf::Processor*, int> SoundEngine::getParameterInfo (const std::string& value) {
      std::stringstream ss(value);
      std::string proc_string;
      std::getline(ss,proc_string,'_');

      auto proc = getLEAFProcessor(proc_string);
       int procID = proc->processorTypeID;
      std::string param_string;
      std::getline(ss,param_string,'_');
      int index = -1;
      auto it = std::find(paramsAllArray[procID].cbegin(), paramsAllArray[procID].cend(),param_string);
      if (it != paramsAllArray[procID].cend()) {
          // Calculate the index
          index = static_cast<int>(std::distance(paramsAllArray[procID].cbegin(), it));
      }

      return {proc, index};
  }

//  leaf::tProcessor * SoundEngine::getLEAFProcessor (const std::string& proc_string) {
//
//  }

  ProcessorBase* SoundEngine::getProcessorFromUUID (int uuid) {

  }

  ModulatorBase* SoundEngine::getModulatorFromUUID (int uuid) {

  }

  leaf::tProcessor * SoundEngine::getLeafProcessorFromUUID (int uuid) {

  }

void SoundEngine::connectMapping (const electrosynth::mapping_change& change) {
    //check for mapping alread exists
    for (auto modulation : mappings)
    {
        if(change.mapping == modulation)
        {
            DBG("adding modualtion:" + juce::String(change.source) +  " to " + juce::String(modulation->dest_) );
            int sourceIndex = 0;
            auto it = std::find (change.mapping->all_connections_.begin(),change.mapping->all_connections_.end(),change.connection );
            if (it != change.mapping->all_connections_.end() )
            {
                sourceIndex = static_cast<int> (std::distance (change.mapping->all_connections_.begin(), it));
            }
            //set the scale value to point to the backend mapping scaling
            change.connection->scalingValue_ = tMappingAdd(&change.mapping->mapping_, change._source, change._dest, change.dest_param_index, sourceIndex, &leaf);
            change.connection->bipolarOffset = &change.mapping->mapping_.bipolarOffset[sourceIndex];
            return;
        }
    }


//    int sourceIndex = 0;
//    auto it = std::find (change.mapping->all_connections_.begin(),change.mapping->all_connections_.end(),change.connection );
//    if (it != change.mapping->all_connections_.end() )
//    {
//        sourceIndex = static_cast<int> (std::distance (change.mapping->all_connections_.begin(), it));
//    }

    //otherwise this is a new mapping
    //index will be 0 in the mapping
    //set the scale value to point to the backend mapping scaling
    change.connection->scalingValue_ = tMappingAdd(&change.mapping->mapping_, change._source, change._dest, change.dest_param_index, 0, &leaf);
    change.connection->bipolarOffset = &change.mapping->mapping_.bipolarOffset[0];


    mappings.push_back(change.mapping);
    DBG("added new modulatino") ;
}


//returns true if the mapping should be completely removd from process mappings
void SoundEngine::disconnectMapping (const electrosynth::mapping_change& change) {
    MappingWrapper* mappingToRemove = nullptr;
    for (auto modulation : mappings)
    {
        if(change.mapping == modulation)
        {
            DBG("removing modualtion:" + juce::String(change.source) +  " from " + juce::String(modulation->dest_) );
            int sourceIndex = 0;
            auto it = std::find (change.mapping->all_connections_.begin(),change.mapping->all_connections_.end(),change.connection );
            if (it != change.mapping->all_connections_.end() )
            {
                sourceIndex = static_cast<int> (std::distance (change.mapping->all_connections_.begin(), it));
            }
            modulation->mapping_.inSources[sourceIndex] = nullptr;
            modulation->mapping_.scalingValues[sourceIndex] = 0.0f;
            modulation->mapping_.inUUIDS[sourceIndex] = 0.0f;
            //set the scale value to point to the backend mapping scaling
            change.connection->scalingValue_ = nullptr;
            //erase-remove idiom reorders array
            modulation->all_connections_.erase(std::remove (modulation->all_connections_.begin(), modulation->all_connections_.end(), change.connection),modulation->all_connections_.end());
            //use reordered array to update the now out of order mapping
            modulation->reorderMapping();

            if (modulation->mapping_.numUsedSources == 0)
            {
                mappingToRemove = modulation;
            }
            break;

        }
    }
    if(mappingToRemove)
    {
        mappings.erase(std::remove(mappings.begin(), mappings.end(), mappingToRemove),mappings.end());
    }
//
//    DBG("didnt find mapping");
//    jassert(true);
}


} // namespace vital
