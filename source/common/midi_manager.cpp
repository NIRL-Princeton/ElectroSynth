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

#include "midi_manager.h"



namespace {
   constexpr int kMidiControlBits = 7;
   constexpr float kHighResolutionMax = (1 << (2 * kMidiControlBits)) - 1.0f;
   constexpr float kControlMax = (1 << kMidiControlBits) - 1.0f;

   force_inline float toHighResolutionValue(int msb, int lsb) {
       if (lsb < 0)
           return msb / kControlMax;

       return ((msb << kMidiControlBits) + lsb) / kHighResolutionMax;
   }
} // namespace
void sendPresetOverMidi(const leaf::tProcessorPreset7Bit& preset, size_t maxChunkSize, juce::MidiOutput* midi_output)
{
    static std::array<std::byte, sizeof(leaf::tProcessorPreset7Bit)> buffer{};
    std::memcpy(buffer.data(), &preset, sizeof(preset));

    std::vector<juce::Span<std::byte>> spans;

    const size_t totalSize = sizeof(leaf::tProcessorPreset7Bit);
    constexpr size_t headerSize =  sizeof(leaf::tProcessorPreset7Bit) - (5 *MAX_NUM_PARAMS);
    const size_t paramBytes = totalSize - headerSize;
    // Check that the header fits into a sysex with our tag and the sysex tags on the beginning and end
    if (headerSize + 3 > maxChunkSize)
    {
        // Invalid: header cannot fit
        jassertfalse;
        return;
    }
    // Create a new buffer for the header span, prepending the tag
    std::vector<std::byte> headerSpan(headerSize + 1);
    headerSpan[0] = std::byte{0x01};  // Tag for header
    std::memcpy(headerSpan.data() + 1, buffer.data(), headerSize);
    // Add first span: just the header
    spans.emplace_back(headerSpan.begin(), headerSpan.size());

    // Add param spans, chunked up to maxChunkSize
    size_t offset = headerSize;
    size_t remaining = paramBytes;
    // a span is a non-owning view into a contiguous bit of memory.
    // thus the vector must exist for the lifetime of the span in order for it
    // to hold state
    std::vector<std::vector<std::byte>> paramSpans;

    while (remaining > 0)
    {
        const size_t numParamsInChunk = std::min((maxChunkSize-3)/5, (remaining+3)/5); //don't split floats
        const size_t chunkSize = numParamsInChunk * 5;
        // Create a new buffer for the param span, prepending the tag
        paramSpans.emplace_back(chunkSize + 1);
        paramSpans.back()[0] = std::byte{0x02};  // Tag for params
        std::memcpy(paramSpans.back().data() + 1, buffer.data() + offset, chunkSize);
        spans.emplace_back(paramSpans.back().begin(),paramSpans.back().size());
        offset += chunkSize;
        remaining -= chunkSize;
    }
    for (auto chunk : spans) {
        midi_output->sendMessageNow(juce::MidiMessage::createSysExMessage(chunk));
    }

}
void sendPresetOverMidi(const leaf::tMappingPreset7Bit& preset, size_t maxChunkSize, juce::MidiOutput* midi_output)
{
    static std::array<std::byte, sizeof(leaf::tMappingPreset7Bit)> buffer{};
    std::memcpy(buffer.data(), &preset, sizeof(preset));

    constexpr size_t paramBytes = (2*(5 *MAX_NUM_SOURCES) + (2*MAX_NUM_SOURCES));
    constexpr size_t headerSize =  sizeof(leaf::tMappingPreset7Bit) - paramBytes;

    // Check that the header fits into a sysex with our tag and the sysex tags on the beginning and end
    if (headerSize + 3 > maxChunkSize)
    {
        // Invalid: header cannot fit
        jassertfalse;
        return;
    }
    if ( headerSize + paramBytes > maxChunkSize) {
        //mapping chunking is made to send in one chunk right now.
        //if MAX_NUM_SOURCES expands beyond 6 (assuming headerSize does not expand
        // beyond 6 variables i.e. 12 8 bit chunks) we will need to change how mappings are sent
        jassertfalse;
        return;
    }

    midi_output->sendMessageNow(juce::MidiMessage::createSysExMessage(buffer.data(), headerSize + paramBytes));

}

MidiManager::MidiManager(MidiKeyboardState* keyboard_state, AudioDeviceManager* manager, const ValueTree &v,
   Listener* listener) : tracktion::engine::ValueTreeObjectList<electrosynth::MidiDeviceWrapper>(v),
                         keyboard_state_(keyboard_state),
                         listener_(listener), armed_value_(nullptr),
                         msb_pressure_values_(), msb_slide_values_() , manager(manager){
   //engine_ = synth_->get//engine();
   current_bank_ = -1;
   current_folder_ = -1;
   current_preset_ = -1;
 
   for (int i = 0; i < electrosynth::kNumMidiChannels; ++i) {
       lsb_slide_values_[i] = -1;
       lsb_pressure_values_[i] = -1;
   }

   mpe_enabled_ = false;
   mpe_zone_layout_.setLowerZone(electrosynth::kNumMidiChannels - 1);
   rebuildObjects();
   for(auto obj: objects)
   {
       manager->addMidiInputDeviceCallback(obj->identifier, this);
   }
}

MidiManager::~MidiManager() {
}

void MidiManager::armMidiLearn(std::string name) {
   current_bank_ = -1;
   current_folder_ = -1;
   current_preset_ = -1;
   //  armed_value_ = &electrosynth::Parameters::getDetails(name);
}

void MidiManager::cancelMidiLearn() {
   armed_value_ = nullptr;
}

//void MidiManager::clearMidiLearn(const std::string& name) {
//  for (auto& controls : midi_learn_map_) {
//    if (controls.second.count(name)) {
//      midi_learn_map_[controls.first].erase(name);
//      LoadSave::saveMidiMapConfig(this);
//    }
//  }
//}

void MidiManager::midiInput(int midi_id, float value) {
   //  if (armed_value_) {
   //    midi_learn_map_[midi_id][armed_value_->name] = armed_value_;
   //    armed_value_ = nullptr;
   //
   //    // TODO: Probably shouldn't write this config on the audio thread.
   //    LoadSave::saveMidiMapConfig(this);
   //  }

   //  if (midi_learn_map_.count(midi_id)) {
   //    for (auto& control : midi_learn_map_[midi_id]) {
   //      const electrosynth::ValueDetails* details = control.second;
   //      float percent = value / kControlMax;
   //      float range = details->max - details->min;
   //      float translated = percent * range + details->min;
   //
   //      if (details->value_scale == electrosynth::ValueDetails::kIndexed)
   //        translated = std::round(translated);
   //      listener_->valueChangedThroughMidi(control.first, translated);
   //    }
   //  }
}

bool MidiManager::isMidiMapped(const std::string& name) const {
   for (auto& controls : midi_learn_map_) {
       if (controls.second.count(name))
           return true;
   }
   return false;
}

void MidiManager::setSampleRate(double sample_rate) {
   midi_collector_.reset(sample_rate);
}

void MidiManager::removeNextBlockOfMessages(MidiBuffer& buffer, int num_samples) {
   midi_collector_.removeNextBlockOfMessages(buffer, num_samples);
}

void MidiManager::readMpeMessage(const MidiMessage& message) {
   mpe_zone_layout_.processNextMidiEvent(message);
}

void MidiManager::processAllNotesOff(const MidiMessage& midi_message, int sample_position, int channel) {
   //  if (isMpeChannelMasterLowerZone(channel))
   //    //engine_->allNotesOffRange(sample_position, lowerZoneStartChannel(), lowerZoneEndChannel());
   //  else if (isMpeChannelMasterUpperZone(channel))
   //    //engine_->allNotesOffRange(sample_position, upperZoneStartChannel(), upperZoneEndChannel());
   //  else
   //    //engine_->allNotesOff(sample_position, channel);
}

void MidiManager::processAllSoundsOff() {
   //engine_->allSoundsOff();
}

void MidiManager::processSustain(const MidiMessage& midi_message, int sample_position, int channel) {
   bool on = midi_message.isSustainPedalOn();
   if (isMpeChannelMasterLowerZone(channel)) {
       //if (on)
       //engine_->sustainOnRange(lowerZoneStartChannel(), lowerZoneEndChannel());
       //else
       //engine_->sustainOffRange(sample_position, lowerZoneStartChannel(), lowerZoneEndChannel());
   }
   else if (isMpeChannelMasterUpperZone(channel)) {
       //if (on)
       //engine_->sustainOnRange(upperZoneStartChannel(), upperZoneEndChannel());
       //else
       //engine_->sustainOffRange(sample_position, upperZoneStartChannel(), upperZoneEndChannel());
   }
   else {
       //if (on)
       //engine_->sustainOn(channel);
       //else
       //engine_->sustainOff(sample_position, channel);
   }
}

void MidiManager::processSostenuto(const MidiMessage& midi_message, int sample_position, int channel) {
   bool on = midi_message.isSostenutoPedalOn();
   if (isMpeChannelMasterLowerZone(channel)) {
       //if (on)
       //engine_->sostenutoOnRange(lowerZoneStartChannel(), lowerZoneEndChannel());
       //else
       //engine_->sostenutoOffRange(sample_position, lowerZoneStartChannel(), lowerZoneEndChannel());
   }
   else if (isMpeChannelMasterUpperZone(channel)) {
       //if (on)
       //engine_->sostenutoOnRange(upperZoneStartChannel(), upperZoneEndChannel());
       //else
       //engine_->sostenutoOffRange(sample_position, upperZoneStartChannel(), upperZoneEndChannel());
   }
   else {
       // if (on)
       //engine_->sostenutoOn(channel);
       // else
       //engine_->sostenutoOff(sample_position, channel);
   }
}

void MidiManager::processPitchBend(const MidiMessage& midi_message, int sample_position, int channel) {
   float percent = midi_message.getPitchWheelValue() / kHighResolutionMax;
   float value = 2 * percent - 1.0f;

   if (isMpeChannelMasterLowerZone(channel)) {
       //engine_->setZonedPitchWheel(value, lowerMasterChannel(), lowerMasterChannel() + 1);
       //engine_->setZonedPitchWheel(value, lowerZoneStartChannel(), lowerZoneEndChannel());
       listener_->pitchWheelMidiChanged(value);
   }
   else if (isMpeChannelMasterUpperZone(channel)) {
       //engine_->setZonedPitchWheel(value, upperMasterChannel(), upperMasterChannel() + 1);
       //engine_->setZonedPitchWheel(value, upperZoneStartChannel(), upperZoneEndChannel());
       listener_->pitchWheelMidiChanged(value);
   }
   //else if (mpe_enabled_)
   //engine_->setPitchWheel(value, channel);
   // else {
   //engine_->setZonedPitchWheel(value, channel, channel);
   // listener_->pitchWheelMidiChanged(value);
   //}
}

void MidiManager::processPressure(const MidiMessage& midi_message, int sample_position, int channel) {
   float value = toHighResolutionValue(msb_pressure_values_[channel], lsb_pressure_values_[channel]);
   // if (isMpeChannelMasterLowerZone(channel))
   //engine_->setChannelRangeAftertouch(lowerZoneStartChannel(), lowerZoneEndChannel(), value, 0);
   // else if (isMpeChannelMasterUpperZone(channel))
   //engine_->setChannelRangeAftertouch(upperZoneStartChannel(), upperZoneEndChannel(), value, 0);
   // else
   //engine_->setChannelAftertouch(channel, value, sample_position);
}

void MidiManager::processSlide(const MidiMessage& midi_message, int sample_position, int channel) {
   float value = toHighResolutionValue(msb_slide_values_[channel], lsb_slide_values_[channel]);
   //if (isMpeChannelMasterLowerZone(channel))
   //engine_->setChannelRangeSlide(value, lowerZoneStartChannel(), lowerZoneEndChannel(), 0);
   //else if (isMpeChannelMasterUpperZone(channel))
   //engine_->setChannelRangeSlide(value, upperZoneStartChannel(), upperZoneEndChannel(), 0);
   // else
   //engine_->setChannelSlide(channel, value, sample_position);
}

force_inline bool MidiManager::isMpeChannelMasterLowerZone(int channel) {
   return mpe_enabled_ && mpe_zone_layout_.getLowerZone().isActive() && lowerMasterChannel() == channel;
}

force_inline bool MidiManager::isMpeChannelMasterUpperZone(int channel) {
   return mpe_enabled_ && mpe_zone_layout_.getUpperZone().isActive() && upperMasterChannel() == channel;
}

void MidiManager::processMidiMessage(const MidiMessage& midi_message, int sample_position) {
   if (midi_message.isController())
       readMpeMessage(midi_message);

   int channel = midi_message.getChannel() - 1;
   MidiMainType type = static_cast<MidiMainType>(midi_message.getRawData()[0] & 0xf0);
   switch (type) {
       case kProgramChange:
           return;
       case kNoteOn: {
           uint8 velocity = midi_message.getVelocity();
           //  if (velocity)
           //engine_->noteOn(midi_message.getNoteNumber(), velocity / kControlMax, sample_position, channel);
           //  else
           //engine_->noteOff(midi_message.getNoteNumber(), velocity / kControlMax, sample_position, channel);
           return;
       }
       case kNoteOff: {
           float velocity = midi_message.getVelocity() / kControlMax;
           //engine_->noteOff(midi_message.getNoteNumber(), velocity, sample_position, channel);
           return;
       }
       case kAftertouch: {
           int note = midi_message.getNoteNumber();
           float value = midi_message.getAfterTouchValue() / kControlMax;
           //engine_->setAftertouch(note, value, sample_position, channel);
           return;
       }
       case kChannelPressure: {
           msb_pressure_values_[channel] = midi_message.getChannelPressureValue();
           processPressure(midi_message, sample_position, channel);
           return;
       }
       case kPitchWheel: {
           processPitchBend(midi_message, sample_position, channel);
           return;
       }
       case kController: {
           MidiSecondaryType secondary_type = static_cast<MidiSecondaryType>(midi_message.getControllerNumber());
           switch (secondary_type) {
               case kSlide: {
                   msb_slide_values_[channel] = midi_message.getControllerValue();
                   processSlide(midi_message, sample_position, channel);
                   break;
               }
               case kLsbPressure: {
                   lsb_pressure_values_[channel] = midi_message.getControllerValue();
                   processPressure(midi_message, sample_position, channel);
                   break;
               }
               case kLsbSlide: {
                   lsb_slide_values_[channel] = midi_message.getControllerValue();
                   processSlide(midi_message, sample_position, channel);
                   break;
               }
               case kSustainPedal: {
                   processSustain(midi_message, sample_position, channel);
                   break;
               }
               case kSostenutoPedal: {
                   processSostenuto(midi_message, sample_position, channel);
                   break;
               }
               case kSoftPedalOn: // TODO
                   break;
               case kModWheel: {
                   float percent = (1.0f * midi_message.getControllerValue()) / kControlMax;
                   //engine_->setModWheel(percent, channel);
                   listener_->modWheelMidiChanged(percent);
                   break;
               }
               case kAllNotesOff:
               case kAllControllersOff:
                   processAllNotesOff(midi_message, sample_position, channel);
                   return;
               case kAllSoundsOff:
                   processAllSoundsOff();
                   break;
               case kBankSelect:
                   current_bank_ = midi_message.getControllerValue();
                   return;
               case kFolderSelect:
                   current_folder_ = midi_message.getControllerValue();
                   return;
           }
           midiInput(midi_message.getControllerNumber(), midi_message.getControllerValue());
       }
   }
}

void MidiManager::handleIncomingMidiMessage(MidiInput* source, const MidiMessage &midi_message) {
   midi_collector_.addMessageToQueue(midi_message);
}

void MidiManager::replaceKeyboardMessages(MidiBuffer& buffer, int num_samples) {
   keyboard_state_->processNextMidiBuffer(buffer, 0, num_samples, true);
}
