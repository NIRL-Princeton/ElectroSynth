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
#include "RoutingProcessor.h"

#include "synth_base.h"
#include "synth_gui_interface.h"
#include "melatonin_audio_sparklines/melatonin_audio_sparklines.h"

#include "startup.h"
#include "../synthesis/synth_engine/sound_engine.h"

#include "Identifiers.h"
#include "load_save.h"
#include "SimpleOscModule.h"
#include "Modulators/ModulatorBase.h"
#include "ModulationWrapper.h"
#include "Processors/ProcessorBase.h"
#include <chowdsp_dsp_data_structures/chowdsp_dsp_data_structures.h>

#include "MasterVoiceProcessor.h"
#include "parameterArrays.h"
#include "Modulators/EnvModuleProcessor.h"
#include "EffectList.h"
#include "EffectOrderPlacement.h"
#include <algorithm>
#include <iterator>

SynthBase::SynthBase(AudioDeviceManager *deviceManager) : tree(ValueTree(IDs::ELECTROSYNTH)), manager(deviceManager) {
    tree.addChild(juce::ValueTree{IDs::CHAINS}, -1, nullptr);
    tree.addChild(juce::ValueTree{IDs::MODULATORS}, -1, nullptr);
    juce::ValueTree effect1{IDs::EFFECTS};
    effect1.setProperty(IDs::effect_lane,0,nullptr);
    juce::ValueTree effect2{IDs::EFFECTS};
    effect2.setProperty(IDs::effect_lane,1,nullptr);
    juce::ValueTree effect3{IDs::EFFECTS};
    effect3.setProperty(IDs::effect_lane,2,nullptr);
    tree.addChild(effect1, -1, nullptr);
    tree.addChild(effect2, -1, nullptr);
    tree.addChild(effect3, -1, nullptr);
    processors_ = std::make_unique<ChainList<ProcessorBase> >(this,tree.getChildWithName(IDs::CHAINS));
    modulators_ = std::make_unique<ModuleList<ModulatorBase> >(this,tree.getChildWithName(IDs::MODULATORS));
    effects_0 = std::make_unique<EffectList >(this,tree.getChildWithProperty(IDs::effect_lane,0),0);
    effects_1 = std::make_unique<EffectList >(this,tree.getChildWithProperty(IDs::effect_lane,1),1);
    effects_2 = std::make_unique<EffectList >(this,tree.getChildWithProperty(IDs::effect_lane,2),2);
    self_reference_ = std::make_shared<SynthBase *>();
    *self_reference_ = this;

    engine_ = std::make_unique<electrosynth::SoundEngine>(um);

    mod_connections_.reserve(electrosynth::kMaxModulationConnections);

    keyboard_state_ = std::make_unique<MidiKeyboardState>();
    ValueTree v;
    midi_manager_ = std::make_unique<MidiManager>(this->getEngine(), keyboard_state_.get(), manager, v, this);

    last_played_note_ = 0.0f;
    last_num_pressed_ = 0;

    Startup::doStartupChecks();

    tree.appendChild(engine_->MasterVoiceEnvelopeProcessor->state, nullptr);
    tree.addListener(this);
    startTimer(500);
}

SynthBase::~SynthBase() {
    stopTimer();
    tree.removeListener(this);
    processors_.reset();
    modulators_.reset();
}

LEAF *SynthBase::getLeaf() {
    return &engine_->leaf;
}

//void SynthBase::valueChanged(const std::string& name, float value) {
//
//}
//
//void SynthBase::valueChangedInternal(const std::string& name, float value) {
//  valueChanged(name, value);
//  setValueNotifyHost(name, value);
//}

//void SynthBase::valueChangedThroughMidi(const std::string& name, float value) {
//
//  ValueChangedCallback* callback = new ValueChangedCallback(self_reference_, name, value);
//  setValueNotifyHost(name, value);
//  callback->post();
//}

void SynthBase::pitchWheelMidiChanged(float value) {
    ValueChangedCallback *callback = new ValueChangedCallback(self_reference_, "pitch_wheel", value);
    callback->post();
}

void SynthBase::modWheelMidiChanged(float value) {
    ValueChangedCallback *callback = new ValueChangedCallback(self_reference_, "mod_wheel", value);
    callback->post();
}

void SynthBase::pitchWheelGuiChanged(float value) {
    engine_->setZonedPitchWheel(value, 0, electrosynth::kNumMidiChannels - 1);
}

void SynthBase::modWheelGuiChanged(float value) {
    engine_->setModWheelAllChannels(value);
}

void SynthBase::presetChangedThroughMidi(File preset) {
    SynthGuiInterface *gui_interface = getGuiInterface();
    if (gui_interface) {
        gui_interface->updateFullGui();
        gui_interface->notifyFresh();
    }
}

//
//void SynthBase::valueChangedExternal(const std::string& name, float value) {
//  valueChanged(name, value);
//  if (name == "mod_wheel")
//    engine_->setModWheelAllChannels(value);
//  else if (name == "pitch_wheel")
//    engine_->setZonedPitchWheel(value, 0, electrosynth::kNumMidiChannels - 1);
//
//  ValueChangedCallback* callback = new ValueChangedCallback(self_reference_, name, value);
//  callback->post();
//}


int SynthBase::getNumModulations(const std::string &destination) {
    int connections = 0;
    for (electrosynth::ModulationConnection *connection: mod_connections_) {
        if (connection->destination_name == destination)
            connections++;
    }
    return connections;
}


void SynthBase::initEngine() {
    checkOversampling();
}


void SynthBase::setMpeEnabled(bool enabled) {
    midi_manager_->setMpeEnabled(enabled);
}
void SynthBase::removeEffect(ProcessorBase *processor, int lane) {
    if (engine_ == nullptr || processor == nullptr) return;

    if (lane < 0 || lane >= static_cast<int>(engine_->effects.size()))
        return; // invalid lane index

    disconnectModulationsForDestinationProcessor(processor->name.toStdString()); // disconnect modulation connections from this FX

    auto& effectLane = engine_->effects[lane];
    auto it = std::find_if(effectLane.begin(), effectLane.end(),
                               [processor](const auto& ptr){
                                   return ptr.get() == processor;
                               });
        if (it != effectLane.end()) {
            // Transfer ownership out before erasing
            std::unique_ptr<ProcessorBase> released = std::move(*it);
            *effectLane.erase(it);
            // Create task as a std::function
            DeleteThreadAction task = [ptr = std::move(released)]() mutable {
                ptr.reset(); // optional; unique_ptr will go out of scope
            };

            // Try enqueue
            if (!processorDeleteQueue.try_enqueue(std::move(task))) {
                jassertfalse;
            }

            return;

    }
}
void SynthBase::removeProcessor(ProcessorBase *processor) {
    if (engine_ == nullptr) return;
    for (auto &chain: engine_->processors) {
        auto it = std::find_if(chain.begin(), chain.end(),
                               [&](const std::unique_ptr<ProcessorBase> &proc) {
                                   return proc.get() == processor;
                               });

        if (it != chain.end())
        {
            auto* chainPtr = &chain;     // pointer to the actual chain object
            auto* target   = processor;  // raw pointer identity

            auto task_ = [this, chainPtr, target]() mutable
            {
                // Re-find inside the task (DO NOT capture iterators)
                auto it2 = std::find_if(chainPtr->begin(), chainPtr->end(),
                                        [&](const auto& up) { return up.get() == target; });

                if (it2 == chainPtr->end())
                    return; // already removed

                // Move out then erase (correct order)
                auto released = std::move(*it2);
                it2->reset(); // leave empty slot; size unchanged
                // IMPORTANT: this next part only works if DeleteThreadAction is move-only.
                // If DeleteThreadAction is std::function<void()>, this will NOT compile.
                DeleteThreadAction del = [ptr = std::move(released)]() mutable {
                    ptr.reset();
                };

                if (!processorDeleteQueue.try_enqueue(std::move(del)))
                    jassertfalse;
            };

            processorInitQueue.try_enqueue(std::move(task_));
            return;
        }
    }
}

void SynthBase::removeProcessor(ModulatorBase *processor) {
    for (auto &chain: engine_->modSources) {
        auto it = std::find_if(chain.begin(), chain.end(),
                               [&](const std::unique_ptr<ModulatorBase> &proc) {
                                   return proc.get() == processor;
                               });

        if (it != chain.end())
        {
            auto* chainPtr = &chain;     // pointer to the actual chain object
            auto* target   = processor;  // raw pointer identity

            auto task_ = [this, chainPtr, target]() mutable
            {
                // Re-find inside the task (DO NOT capture iterators)
                auto it2 = std::find_if(chainPtr->begin(), chainPtr->end(),
                                        [&](const auto& up) { return up.get() == target; });

                if (it2 == chainPtr->end())
                    return; // already removed

                auto released = std::move(*it2);
                it2->reset(); // leave empty slot; size unchanged

                // IMPORTANT: this next part only works if DeleteThreadAction is move-only.
                // If DeleteThreadAction is std::function<void()>, this will NOT compile.
                DeleteThreadAction del = [ptr = std::move(released)]() mutable {
                    ptr.reset();
                };

                if (!processorDeleteQueue.try_enqueue(std::move(del)))
                    jassertfalse;
            };

            processorInitQueue.try_enqueue(std::move(task_));
            return;
        }

    }
}

void SynthBase::removeChainRouting(RoutingProcessor *processor) {

    if (engine_ == nullptr) return;

    auto& post = engine_->chainPostGain;
    auto it = std::find_if(post.begin(), post.end(),
                           [&](const std::unique_ptr<RoutingProcessor>& p)
                           {
                               return p.get() == processor;
                           });

    if (it == post.end())
        return;
    const size_t idx = static_cast<size_t>(std::distance(post.begin(), it));

    auto* postPtr   = &engine_->chainPostGain;
    auto* chainsPtr = &engine_->processors;   // <-- the parallel structure
    auto* target    = processor;

    auto task_ = [this, postPtr, chainsPtr, idx, target]() mutable

    {
        if (idx >= postPtr->size() || idx >= chainsPtr->size())
            return;
        std::unique_ptr<RoutingProcessor> released = std::move((*postPtr)[idx]);
        (*chainsPtr)[idx].clear();
        DeleteThreadAction del = [ptr = std::move(released)]() mutable { ptr.reset(); };

        if (!processorDeleteQueue.try_enqueue(std::move(del)))
            jassertfalse;
    };

        processorInitQueue.try_enqueue(std::move(task_));
}





void SynthBase::addChainRouting(std::unique_ptr<RoutingProcessor> processor, int chain_index) {
    processor->prepareToPlay(engine_->getSampleRate(), engine_->getBufferSize());

    engine_->chainPostGain[chain_index]=std::move(processor);
}
void SynthBase::addProcessor(std::unique_ptr<ProcessorBase> processor, int chain_index) {
    processor->prepareToPlay(engine_->getSampleRate(), engine_->getBufferSize());
    auto proc0 = processor->procArray[0];
    std::atomic<float> *watchParameter = (proc0[0])->params[EVENT_WATCH_INDEX];

    if (*proc0[0]->params[EVENT_WATCH_INDEX] == 1) {
        for (int i = 0; i < MAX_NUM_VOICES; i++)
            engine_->voiceHandler.eventEmitter.listeners[i][engine_->voiceHandler.eventEmitter.numListeners] =
                    processor->procArray->at(i);
    }
    engine_->voiceHandler.eventEmitter.numListeners++;
    engine_->processors[chain_index].push_back(std::move(processor));
}
void SynthBase::addEffect(std::unique_ptr<ProcessorBase> processor, int lane) {
    processor->prepareToPlay(engine_->getSampleRate(), engine_->getBufferSize());
    auto proc0 = processor->procArray[0];
    std::atomic<float> *watchParameter = (proc0[0])->params[EVENT_WATCH_INDEX];

    if (*proc0[0]->params[EVENT_WATCH_INDEX] == 1) {
        for (int i = 0; i < MAX_NUM_VOICES; i++)
            engine_->voiceHandler.eventEmitter.listeners[i][engine_->voiceHandler.eventEmitter.numListeners] =
                    processor->procArray->at(i);
    }
    engine_->voiceHandler.eventEmitter.numListeners++;
    engine_->effects[lane].push_back(std::move(processor));
}

void SynthBase::submitEffectOrder(int lane, ProcessorBase* movedProcessor, ProcessorBase* nextProcessor) {
    if (movedProcessor == nullptr)
        return;

    EffectOrderCommand command {
        lane,
        -1,
        movedProcessor,
        nextProcessor,
        nextEffectOrderGeneration_++
    };

    if (!pendingEffectOrderCommands_.empty() || !effectOrderQueue_.try_enqueue(command))
        pendingEffectOrderCommands_.push_back(command);
}

void SynthBase::submitEffectMove(int sourceLane, int targetLane,
                                 ProcessorBase* movedProcessor,
                                 ProcessorBase* nextProcessor) {
    if (movedProcessor == nullptr || sourceLane == targetLane)
        return;

    EffectOrderCommand command {
        sourceLane,
        targetLane,
        movedProcessor,
        nextProcessor,
        nextEffectOrderGeneration_++
    };
    if (!pendingEffectOrderCommands_.empty() || !effectOrderQueue_.try_enqueue(command))
        pendingEffectOrderCommands_.push_back(command);
}

void SynthBase::registerEffectList(EffectList* effectList) {
    if (effectList == nullptr)
        return;

    if (std::find(registeredEffectLists_.begin(), registeredEffectLists_.end(), effectList) == registeredEffectLists_.end())
        registeredEffectLists_.push_back(effectList);
}

void SynthBase::unregisterEffectList(EffectList* effectList) {
    registeredEffectLists_.erase(
        std::remove(registeredEffectLists_.begin(), registeredEffectLists_.end(), effectList),
        registeredEffectLists_.end());
}

void SynthBase::flushPendingEffectOrderCommands() {
    while (!pendingEffectOrderCommands_.empty()) {
        if (!effectOrderQueue_.try_enqueue(pendingEffectOrderCommands_.front()))
            break;
        pendingEffectOrderCommands_.pop_front();
    }
}

void SynthBase::reconcileEffectOrders() {
    for (auto* effectList : registeredEffectLists_)
        if (effectList != nullptr)
            effectList->publishCurrentOrder();
}

bool SynthBase::applyEffectOrderCommand(const EffectOrderCommand& command) {
    if (engine_ == nullptr || command.lane < 0 || command.lane >= static_cast<int>(engine_->effects.size()))
        return false;

    auto& effectLane = engine_->effects[static_cast<std::size_t>(command.lane)];
    if (command.targetLane >= 0) {
        if (command.targetLane >= static_cast<int>(engine_->effects.size())
            || command.targetLane == command.lane)
            return false;

        auto moved = std::find_if(effectLane.begin(), effectLane.end(),
                                  [&command](const auto& processor) {
                                      return processor.get() == command.movedProcessor;
                                  });
        auto& targetLane = engine_->effects[static_cast<std::size_t>(command.targetLane)];
        auto insertion = command.nextProcessor == nullptr
                           ? targetLane.end()
                           : std::find_if(targetLane.begin(), targetLane.end(),
                                          [&command](const auto& processor) {
                                              return processor.get() == command.nextProcessor;
                                          });
        if (moved == effectLane.end()
            || (command.nextProcessor != nullptr && insertion == targetLane.end()))
            return false;

        auto ownedProcessor = std::move(*moved);
        effectLane.erase(moved);
        targetLane.insert(insertion, std::move(ownedProcessor));
        return true;
    }

    return electrosynth::effect_order::placeBefore(
               effectLane, command.movedProcessor, command.nextProcessor)
           == electrosynth::effect_order::PlacementResult::applied;
}

void SynthBase::completeEffectOrderCommand(const EffectOrderCommand& command) {
    const auto applied = applyEffectOrderCommand(command);
    if (applied) {
        lastAdoptedEffectOrderGeneration_.store(command.generation, std::memory_order_release);
    } else {
        rejectedEffectOrderCommandCount_.fetch_add(1, std::memory_order_relaxed);
        effectOrderReconciliationRequested_.store(true, std::memory_order_release);
    }
}

void SynthBase::drainEffectOrderQueue() {
    if (engine_ == nullptr)
        return;

    if (!activeEffectOrderCommand_.has_value()) {
        EffectOrderCommand nextCommand;
        if (deferredEffectOrderCommand_.has_value()) {
            nextCommand = *deferredEffectOrderCommand_;
            deferredEffectOrderCommand_.reset();
        } else if (!effectOrderQueue_.try_dequeue(nextCommand)) {
            return;
        }

        if (nextCommand.lane < 0
            || nextCommand.lane >= static_cast<int>(engine_->effects.size())) {
            completeEffectOrderCommand(nextCommand);
            return;
        }

        activeEffectOrderCommand_ = nextCommand;
        engine_->beginEffectLaneFadeOut(nextCommand.lane);
        if (nextCommand.targetLane >= 0)
            engine_->beginEffectLaneFadeOut(nextCommand.targetLane);
        return;
    }

    const auto transitionLane = activeEffectOrderCommand_->lane;
    const int targetTransitionLane = activeEffectOrderCommand_->targetLane;
    if (!engine_->isEffectLaneSilent(transitionLane)
        || (targetTransitionLane >= 0
            && !engine_->isEffectLaneSilent(targetTransitionLane)))
        return;

    completeEffectOrderCommand(*activeEffectOrderCommand_);

    EffectOrderCommand nextCommand;
    if (targetTransitionLane < 0) {
        while (effectOrderQueue_.try_dequeue(nextCommand)) {
            if (nextCommand.lane != transitionLane || nextCommand.targetLane >= 0) {
                deferredEffectOrderCommand_ = nextCommand;
                break;
            }
            completeEffectOrderCommand(nextCommand);
        }
    }

    activeEffectOrderCommand_.reset();
    engine_->beginEffectLaneFadeIn(transitionLane);
    if (targetTransitionLane >= 0)
        engine_->beginEffectLaneFadeIn(targetTransitionLane);
}

void SynthBase::addModulationSource(std::unique_ptr<ModulatorBase> modulationSource, int voice_index) {
    modulationSource->prepareToPlay(engine_->getBufferSize(), engine_->getSampleRate());

    ModuleHeader* proc0 = modulationSource->procArray->at(0);//[0];
    std::atomic<float> *watchParameter = (proc0)->params[EVENT_WATCH_INDEX];

    if (*proc0->params[EVENT_WATCH_INDEX] == 1) {
        for (int i = 0; i < MAX_NUM_VOICES; i++)
            engine_->voiceHandler.eventEmitter.listeners[i][engine_->voiceHandler.eventEmitter.numListeners] =
                    modulationSource->procArray->at(i);
    }
    engine_->voiceHandler.eventEmitter.numListeners++;
    engine_->modSources[voice_index].push_back(std::move(modulationSource));
}

bool SynthBase::loadFromValueTree(const ValueTree &state) {
    pauseProcessing(true);
    engine_->allSoundsOff();
    tree.copyPropertiesAndChildrenFrom(state, nullptr);
    pauseProcessing(false);
    //DBG("unpause processing");
    if (tree.isValid())
        return true;
    return false;
}

bool SynthBase::loadFromFile(File preset, std::string &error) {
    //DBG("laoding from file");
    if (!preset.exists())
        return false;

    auto xml = juce::parseXML(preset);
    if (xml == nullptr) {
        error = "Error loading preset";
        return false;
    }
    auto parsed_value_tree = ValueTree::fromXml(*xml);
    if (!parsed_value_tree.isValid()) {
        error = "Error converting XML to ValueTree";
        return false;
    }

    SynthGuiInterface *gui_interface = getGuiInterface();

    // clear everything and update the GUI
    // if (gui_interface) {
    //     gui_interface->updateFullGui();
    //     gui_interface->notifyFresh();
    // }

    // load your new value tree and update the GUI
    if (!loadFromValueTree(parsed_value_tree)) {
        error = "Error Initializing ValueTree";
        return false;
    }
    if (gui_interface) {
        gui_interface->updateFullGui();
        gui_interface->notifyFresh();
    }
    //setPresetName(preset.getFileNameWithoutExtension());

    return true;
}

bool SynthBase::saveToFile(File preset) {
    preset = preset.withFileExtension(String(electrosynth::kPresetExtension));

    File parent = preset.getParentDirectory();
    if (!parent.exists()) {
        if (!parent.createDirectory().wasOk() || !parent.hasWriteAccess())
            return false;
    }

    setPresetName(preset.getFileNameWithoutExtension());

    SynthGuiInterface *gui_interface = getGuiInterface();
    if (gui_interface)
        gui_interface->notifyFresh();

    //    if (preset.replaceWithText(saveToJson().dump())) {
    //        active_file_ = preset;
    //        return true;
    //    }
    return false;
}

bool SynthBase::saveToActiveFile() {
    if (!active_file_.exists() || !active_file_.hasWriteAccess())
        return false;

    return saveToFile(active_file_);
}


void SynthBase::processAudio(AudioSampleBuffer *buffer, int channels, int samples, int offset) {
    AudioThreadAction action;
    while (processorInitQueue.try_dequeue(action))
        action();
    drainEffectOrderQueue();
    processMappingChanges();
    engine_->process(*buffer, channels, samples, offset);
    //writeAudio(buffer, channels, samples, offset);
}

void SynthBase::processAudioAndMidi(juce::AudioBuffer<float> &audio_buffer, juce::MidiBuffer &midi_buffer)
//, int channels, int samples, int offset, int start_sample = 0, int end_sample = 0)
{
    AudioThreadAction action;
    while (processorInitQueue.try_dequeue(action))
        action();
    drainEffectOrderQueue();
    processMappingChanges();

    engine_->process(audio_buffer, midi_buffer);

    //melatonin::printSparkline(audio_buffer);
}

void SynthBase::processAudioWithInput(AudioSampleBuffer *buffer, const float *input_buffer,
                                      int channels, int samples, int offset) {
    engine_->processWithInput(input_buffer, samples);
    writeAudio(buffer, channels, samples, offset);
}

void SynthBase::writeAudio(AudioSampleBuffer *buffer, int channels, int samples, int offset) {
    //const float* engine_output = (const float*)engine_->output(0)->buffer;
    /* get output of engine here */
    for (int channel = 0; channel < channels; ++channel) {
        float *channel_data = buffer->getWritePointer(channel, offset);
        //this line actually sends audio to the JUCE AudioSamplerBuffer to get audio out of the plugin
        for (int i = 0; i < samples; ++i) {
            //channel_data[i] = engine_output[float::kSize * i + channel];
            _ASSERT(std::isfinite(channel_data[i]));
        }
    }
    /*this line would send audio out to draw and get info from */
    //updateMemoryOutput(samples, engine_->output(0)->buffer);
}

void SynthBase::processMidi(MidiBuffer &midi_messages, int start_sample, int end_sample) {
    bool process_all = true;//end_sample == 0;
    for (const MidiMessageMetadata message: midi_messages) {
        int midi_sample = message.samplePosition;
        if (process_all || (midi_sample >= start_sample && midi_sample < end_sample))
            midi_manager_->processMidiMessage(message.getMessage(), midi_sample - start_sample);
    }
}

void SynthBase::processKeyboardEvents(MidiBuffer &buffer, int num_samples) {
    midi_manager_->replaceKeyboardMessages(buffer, num_samples);
}


void SynthBase::updateMemoryOutput(int samples, const float *audio) {
    //  for (int i = 0; i < samples; ++i)
    //    audio_memory_->push(audio[i]);
    //
    //  float last_played = engine_->getLastActiveNote();
    //  last_played = electrosynth::utils::clamp(last_played, kOutputWindowMinNote, kOutputWindowMaxNote);
    //
    //  int num_pressed = engine_->getNumPressedNotes();
    //  int output_inc = std::max<int>(1, engine_->getSampleRate() / electrosynth::kOscilloscopeMemorySampleRate);
    //  int oscilloscope_samples = 2 * electrosynth::kOscilloscopeMemoryResolution;
    //
    //  if (last_played && (last_played_note_ != last_played || num_pressed > last_num_pressed_)) {
    //    last_played_note_ = last_played;
    //
    //    //electrosynth::utils::copyBuffer(oscilloscope_memory_, oscilloscope_memory_write_, oscilloscope_samples);
    //  }
    //  last_num_pressed_ = num_pressed;
    //
    ////  for (; memory_input_offset_ < samples; memory_input_offset_ += output_inc) {
    ////    int input_index = electrosynth::utils::iclamp(memory_input_offset_, 0, samples);
    ////    memory_index_ = electrosynth::utils::iclamp(memory_index_, 0, oscilloscope_samples - 1);
    ////    _ASSERT(input_index >= 0);
    ////    _ASSERT(input_index < samples);
    ////    _ASSERT(memory_index_ >= 0);
    ////    _ASSERT(memory_index_ < oscilloscope_samples);
    ////    //oscilloscope_memory_write_[memory_index_++] = audio[input_index];
    ////
    ////    if (memory_index_ * output_inc >= memory_reset_period_) {
    ////      memory_input_offset_ += memory_reset_period_ - memory_index_ * output_inc;
    ////      memory_index_ = 0;
    ////      //electrosynth::utils::copyBuffer(oscilloscope_memory_, oscilloscope_memory_write_, oscilloscope_samples);
    ////    }
    ////  }
    //
    //  memory_input_offset_ -= samples;
}

//void SynthBase::armMidiLearn(const std::string& name) {
//  midi_manager_->armMidiLearn(name);
//}
//
//void SynthBase::cancelMidiLearn() {
//  midi_manager_->cancelMidiLearn();
//}
//
//void SynthBase::clearMidiLearn(const std::string& name) {
//  midi_manager_->clearMidiLearn(name);
//}

void SynthBase::valueChanged(const std::string &name, float value) {
    //  controls_[name]->set(value);
}


void SynthBase::valueChangedThroughMidi(const std::string &name, float value) {
    //  controls_[name]->set(value);
    //  ValueChangedCallback* callback = new ValueChangedCallback(self_reference_, name, value);
    //  setValueNotifyHost(name, value);
    //  callback->post();
}

int SynthBase::getSampleRate() {
    return engine_->getSampleRate();
}

bool SynthBase::isMidiMapped(const std::string &name) {
    return midi_manager_->isMidiMapped(name);
}

void SynthBase::setAuthor(const String &author) {
    save_info_["author"] = author;
}

void SynthBase::setComments(const String &comments) {
    save_info_["comments"] = comments;
}

void SynthBase::setStyle(const String &style) {
    save_info_["style"] = style;
}

void SynthBase::setPresetName(const String &preset_name) {
    save_info_["preset_name"] = preset_name;
}

void SynthBase::setMacroName(int index, const String &macro_name) {
    save_info_["macro" + std::to_string(index + 1)] = macro_name;
}

String SynthBase::getAuthor() {
    return save_info_["author"];
}

String SynthBase::getComments() {
    return save_info_["comments"];
}

String SynthBase::getStyle() {
    return save_info_["style"];
}

String SynthBase::getPresetName() {
    return save_info_["preset_name"];
}


void SynthBase::notifyOversamplingChanged() {
    pauseProcessing(true);
    engine_->allSoundsOff();
    checkOversampling();
    pauseProcessing(false);
}

void SynthBase::checkOversampling() {
    return engine_->checkOversampling();
}

void SynthBase::ValueChangedCallback::messageCallback() {
    if (auto synth_base = listener.lock()) {
        SynthGuiInterface *gui_interface = (*synth_base)->getGuiInterface();
        if (gui_interface) {
            gui_interface->updateGuiControl(control_name, value);
            if (control_name != "pitch_wheel")
                gui_interface->notifyChange();
        }
    }
}

// juce::ValueTree& SynthBase::getValueTree()
// {
//    return tree;
// }

juce::UndoManager &SynthBase::getUndoManager() {
    return um;
}

/////////////////////// begin modulation and processor queue processing /////////

electrosynth::ModulationConnectionBank &SynthBase::getModulationBank() {
    return engine_->getModulationBank();
}

//this function does not set if it is disconnecting or not. you must do that outside this function
electrosynth::mapping_change SynthBase::createMappingChange(electrosynth::ModulationConnection *connection) {
    electrosynth::mapping_change change {};
    change.connection = connection;
    if (connection == nullptr)
        return change;

    //leaf::Processor* source = engine_->getLEAFProcessor(proc_string);
    std::stringstream ss(connection->source_name);
    std::string proc_string;
    std::getline(ss, proc_string, '_');
    auto [dest, index] = engine_->getParameterInfo(connection->destination_name);
    auto source = engine_->getLEAFProcessorModulator(proc_string);
    connection->sourceProc_ = source;
    change.mapping = connection->mapping_;
    change.destination = connection->destination_name;
    change.dest_param_index = index;
    //change.source_uuid = source->processorUniqueID;

    change._dest = dest;
    change._source = source;
    change.source = proc_string ;

    return change;
}


std::vector<electrosynth::ModulationConnection *> SynthBase::getSourceConnections(const std::string &source) {
    std::vector<electrosynth::ModulationConnection *> connections;
    for (auto &connection: mod_connections_) {
        if (connection->source_name == source)
            connections.push_back(connection);
    }
    return connections;
}

std::vector<electrosynth::ModulationConnection *> SynthBase::getDestinationConnections(const std::string &destination) {
    std::vector<electrosynth::ModulationConnection *> connections;
    for (auto &connection: mod_connections_) {
        if (connection->destination_name == destination)
            connections.push_back(connection);
    }
    return connections;
}

electrosynth::ModulationConnection *SynthBase::getConnection(const std::string &source,
    const std::string &destination, int destination_slot) {
    for (auto &connection: mod_connections_) {
        if (connection->source_name == source
            && connection->destination_name == destination
            && (destination_slot < 0 || connection->destination_slot == destination_slot))
            return connection;
    }
    return nullptr;
}

// does this source already have a connection to this destination?
bool SynthBase::hasSourceDestinationConnection(const std::string &source, const std::string &destination) const
{
    for (auto* existing : mod_connections_)
    {
        if (existing->source_name == source && existing->destination_name == destination) return true;
    }
    return false;
}

bool SynthBase::connectModulation(const std::string &source, const std::string &destination, int destination_slot) {

    electrosynth::ModulationConnection *connection = getConnection(source, destination, destination_slot);
    bool create = connection == nullptr;
    if (create && !hasSourceDestinationConnection (source, destination)) {
        if (destination_slot >= 0) {
            for (auto* existing : mod_connections_) {
                if (existing->destination_name == destination
                    && existing->destination_slot == destination_slot)
                    return false;
            }
        }

        connection = getModulationBank().createConnection(source, destination, destination_slot);
        if (connection == nullptr)
            return false;
        tree.appendChild(connection->state, nullptr);
    }
    if (connection)
        connectModulation(connection);
    return create && connection != nullptr
           && !connection->source_name.empty()
           && !connection->destination_name.empty();
}

void SynthBase::connectModulation(electrosynth::ModulationConnection *connection) {
    electrosynth::mapping_change change = createMappingChange(connection);
    if (isInvalidConnection(change)) {
        if (connection->state.getParent().isValid())
            connection->state.getParent().removeChild(connection->state, nullptr);
        connection->clearConnection();
    } else if (mod_connections_.count(connection) == 0) {
        change.disconnecting = false;
        mod_connections_.push_back(connection);
        connection->mapping_->all_connections_.push_back(connection);
        //push wrapper to actual processors
        modulation_change_queue_.enqueue(change);
    }
}


void SynthBase::disconnectModulation(electrosynth::ModulationConnection *connection) {
    if (mod_connections_.count(connection) == 0)
        return;

    electrosynth::mapping_change change = createMappingChange(connection);
    if (connection->state.getParent().isValid())
        connection->state.getParent().removeChild(connection->state, nullptr);
    connection->clearConnection();

    mod_connections_.remove(connection);
    change.disconnecting = true;
    modulation_change_queue_.enqueue(change);
}

void SynthBase::disconnectModulation(const std::string &source, const std::string &destination) {
    electrosynth::ModulationConnection *connection = getConnection(source, destination);
    if (connection)
        disconnectModulation(connection);
}

// if a filter is deleted, disconnect all modulation connections it is the source of
void SynthBase::disconnectModulationsForDestinationProcessor(const std::string& processor_name) {
    const std::string dest_prefix = processor_name + "_";
    std::vector<electrosynth::ModulationConnection*> connections_to_remove;

    for (auto* connection : mod_connections_) {
        if (connection == nullptr) continue;
        const auto destination = connection->destination_name;

        if (destination.size() >= dest_prefix.size() && destination.starts_with(dest_prefix)) {
            connections_to_remove.push_back(connection);
        }
    }

    for (auto* connection : connections_to_remove) {
        disconnectModulation(connection);
    }
}


void SynthBase::processMappingChanges() {
    electrosynth::mapping_change change;
    while (getNextModulationChange(change)) {
        if (change.disconnecting)
            engine_->disconnectMapping(change);
        else
            engine_->connectMapping(change);
    }
}


//handle deletion
void SynthBase::timerCallback() {
    DeleteThreadAction action;
    while (processorDeleteQueue.try_dequeue(action))
        action();

    if (effectOrderReconciliationRequested_.exchange(false, std::memory_order_acq_rel))
        reconcileEffectOrders();
    flushPendingEffectOrderCommands();

    // bool succeeded = true;
    // while (succeeded) {
    //     auto * front = processorDeleteQueue.peek();
    //     if (front!=nullptr && (*front)()) {
    //         succeeded = processorDeleteQueue.try_dequeue(action);
    //     }else {
    //         succeeded = false;
    //     }
    // }
}
void SynthBase::valueTreePropertyChanged(ValueTree &treeWhosePropertyHasChanged, const Identifier &property) {
    if ( property == IDs::sync) {
        if (treeWhosePropertyHasChanged.getProperty(property) == juce::var{1}) {
            tree.getChildWithName(IDs::CHAINS).setProperty(IDs::sync,1,nullptr);
        }
        treeWhosePropertyHasChanged.removeProperty(IDs::sync,nullptr);
    }
}
