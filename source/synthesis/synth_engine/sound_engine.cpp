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

#include "ModuleGraph.h"
#include "../framework/Processors/OscillatorModuleProcessor.h"
#include "MasterVoiceProcessor.h"
#include "ModulationConnection.h"
#include "ModulationWrapper.h"
#include "Modulators/EnvModuleProcessor.h"
#include "Modulators/ModulatorBase.h"
#include "Processors/ProcessorBase.h"
#include "RoutingProcessor.h"
#include "melatonin_audio_sparklines/melatonin_audio_sparklines.h"
#include "parameterArrays.h"

namespace electrosynth
{
    static void LEAF_errorCallback(LEAF* const leaf, LEAFErrorType error)
    {
        // (void)leaf;

        DBG("LEAF ERROR: " + juce::String(error));
    }
    SoundEngine::SoundEngine (juce::UndoManager& um) : undo (um), /*voice_handler_(nullptr),*/
                                                       last_oversampling_amount_ (-1),
                                                       last_sample_rate_ (-1),
                                                       modulation_bank_ ((leaf))
    {
        moduleGraph_ = std::make_unique<ModuleGraph>();
        // LEAF_init (&leaf, 44100.0f, memory, 536870912, []() { return (float) rand() / RAND_MAX; });
        // //processors.push_back(std::make_shared<OscillatorModuleProcessor> (&leaf));
        // //SoundEngine::init();
        // LEAF_setErrorCallback(&leaf, LEAF_errorCallback);
        // tSimplePoly_create (&leaf.mempool, &voiceHandler.voices[0]);
        // tSimplePoly_init (&leaf, voiceHandler.voices[0], MAX_NUM_VOICES);
        // voiceHandler.numVoicesActive = MAX_NUM_VOICES;
        // tSimplePoly_setNumVoices (voiceHandler.voices[0], (uint8_t) voiceHandler.numVoicesActive);
        // voiceHandler.voiceNote[0] = 0;
        // for (uint8_t i = 1; i < MAX_NUM_VOICES; i++)
        // {
        //     tSimplePoly_create (&leaf.mempool, &voiceHandler.voices[i]);
        //     tSimplePoly_init (&leaf, voiceHandler.voices[i], MAX_NUM_VOICES);
        //     voiceHandler.voiceNote[i] = 0;
        //     voiceHandler.voiceIsSounding[i] = false;
        //     voiceHandler.voicePrevBend[i] = 0.0f;
        // }
        LEAF_init (&leaf, 44100.0f, memory, 536870912, []() { return (float) rand() / RAND_MAX; });
        //processors.push_back(std::make_shared<OscillatorModuleProcessor> (&leaf));
        //SoundEngine::init();
        LEAF_setErrorCallback(&leaf, LEAF_errorCallback);
        tSimplePoly_create (&leaf.mempool, &voiceHandler.voices);
        tSimplePoly_init (&leaf, voiceHandler.voices, MAX_NUM_VOICES);
        voiceHandler.numVoicesActive = MAX_NUM_VOICES;
        tSimplePoly_setNumVoices (voiceHandler.voices, (uint8_t) voiceHandler.numVoicesActive);

        tStack_create (&leaf.mempool, &voiceHandler.voiceOrder);
        tStack_init (&leaf, voiceHandler.voiceOrder);
        tStack_setCapacity (voiceHandler.voiceOrder, MAX_NUM_VOICES);
        for (uint8_t i = 1; i < MAX_NUM_VOICES; i++)
        {
            voiceHandler.voiceNote[i] = 0;
            voiceHandler.voiceIsSounding[i] = false;
            voiceHandler.voicePrevBend[i] = 0.0f;
        }
        processors.resize (10);
        chainPostGain.reserve (10);
        chainPostGain.resize (10);
        for (auto& processor : processors)
        {
            processor.reserve (10);
        }
        modSources.resize (10);
        for (auto& modSource : modSources)
        {
            modSource.reserve (10);
        }
        // Inter-lane moves happen at the fade-to-silence boundary on the audio
        // thread. Reserve normal UI capacity up front so vector insertion does not
        // allocate during that transaction.
        for (auto& effectLane : effects)
            effectLane.reserve(64);

        MasterVoiceEnvelopeProcessor = std::make_unique<EnvModuleProcessor> (
            this, juce::ValueTree (IDs::MODULATOR).setProperty (IDs::type, "env", nullptr), &leaf, &undo);
        MasterVoiceEnvelopeProcessor->state_.params.attackParam->setParameterValue (0.1);
        MasterVoiceEnvelopeProcessor->state_.params.decayParam->setParameterValue (0.01);
        MasterVoiceEnvelopeProcessor->state_.params.releaseParam->setParameterValue (0.001);
        MasterVoiceEnvelopeProcessor->state.setProperty (IDs::name, "master_voice", nullptr);
        //temp_voice_buffer.set
        for (auto& buffer : temp_fx_buffers)
            buffer.setSize (MAX_NUM_VOICES * 2, 1);
    }

    SoundEngine::~SoundEngine()
    {
        // delete[] memory;
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

    int SoundEngine::getNumPressedNotes()
    {
        //return voice_handler_->getNumPressedNotes();
    }

    int SoundEngine::getNumActiveVoices()
    {
        //return voice_handler_->getNumActiveVoices();
    }

    float SoundEngine::getLastActiveNote() const
    {
        //return voice_handler_->getLastActiveNote();
    }

    void SoundEngine::checkOversampling()
    {
        //int oversampling = oversampling_->value();
        // int oversampling_amount = 1 << oversampling;
        //int sample_rate = getSampleRate();
        //    if (last_oversampling_amount_ != oversampling_amount || last_sample_rate_ != sample_rate)
        //      setOversamplingAmount(oversampling_amount, sample_rate);
    }

    void SoundEngine::setOversamplingAmount (int oversampling_amount, int sample_rate)
    {
        static constexpr int kBaseSampleRate = 44100;

        int oversample = oversampling_amount;
        int sample_rate_mult = sample_rate / kBaseSampleRate;
        while (sample_rate_mult > 1 && oversample > 1)
        {
            sample_rate_mult >>= 1;
            oversample >>= 1;
        }
        //voice_handler_->setOversampleAmount(oversample);

        last_oversampling_amount_ = oversampling_amount;
        last_sample_rate_ = sample_rate;
    }

    void SoundEngine::beginEffectLaneFadeOut(int lane) noexcept
    {
        if (juce::isPositiveAndBelow(lane, static_cast<int>(effectLaneTransitions_.size())))
            effectLaneTransitions_[static_cast<std::size_t>(lane)].beginFadeOut();
    }

    void SoundEngine::beginEffectLaneFadeIn(int lane) noexcept
    {
        if (juce::isPositiveAndBelow(lane, static_cast<int>(effectLaneTransitions_.size())))
            effectLaneTransitions_[static_cast<std::size_t>(lane)].beginFadeIn();
    }

    bool SoundEngine::isEffectLaneSilent(int lane) const noexcept
    {
        return juce::isPositiveAndBelow(lane, static_cast<int>(effectLaneTransitions_.size()))
               && effectLaneTransitions_[static_cast<std::size_t>(lane)].isSilent();
    }

    void SoundEngine::registerModule(ModuleBase* module)
    {
        if (module == nullptr)
            return;

        const auto nodeId = module->getNodeId();
        if (nodeId.isEmpty())
            return;

        moduleRegistry_[nodeId] = module;
    }

    void SoundEngine::unregisterModule(ModuleBase* module)
    {
        if (module == nullptr)
            return;

        const auto nodeId = module->getNodeId();
        if (nodeId.isEmpty())
            return;

        auto it = moduleRegistry_.find(nodeId);
        if (it != moduleRegistry_.end() && it->second == module)
            moduleRegistry_.erase(it);
    }

    ModuleBase* SoundEngine::getModuleByNodeId(const juce::String& nodeId) const
    {
        if (nodeId.isEmpty())
            return nullptr;

        const auto it = moduleRegistry_.find(nodeId);
        return it != moduleRegistry_.end() ? it->second : nullptr;
    }

    bool SoundEngine::connectGraphConnection(const electrosynth::ConnectionRecord& connection)
    {
        if (moduleGraph_ == nullptr || !moduleGraph_->connect(connection))
            return false;

#if JUCE_DEBUG
        moduleGraph_->debugPrint("ModuleGraph after connect: " + connection.id,
            [this](const juce::String& nodeId)
            {
                if (auto* module = getModuleByNodeId(nodeId))
                    return module->getDisplayName().isNotEmpty() ? module->getDisplayName() : nodeId;
                return nodeId;
            });
#endif

        if (connection.type != electrosynth::ConnectionType::Modulation)
            return true;

        auto& state = modulationStates_[connection.id];
        state.record = connection;
        state.scalingValue.store(connection.amount, std::memory_order_relaxed);

        std::stringstream source_stream(connection.source.endpointId.toStdString());
        std::string source_token;
        std::getline(source_stream, source_token, '_');
        state.source = getLEAFProcessorModulator(source_token);
        std::tie(state.destination, state.destinationParamIndex) = getParameterInfo(connection.destination.endpointId.toStdString());

        if (state.source == nullptr || state.destination == nullptr || state.destinationParamIndex < 0)
        {
            moduleGraph_->disconnect(connection.id);
            modulationStates_.erase(connection.id);
            return false;
        }

        for (int voice = 0; voice < MAX_NUM_VOICES; ++voice)
        {
            if (state.mapping[voice] == nullptr)
                tMapping_init(&state.mapping[voice], &leaf);

            auto* voice_mapping = state.mapping[voice];
            if (voice_mapping == nullptr
                || !juce::isPositiveAndBelow(voice, static_cast<int>(state.source->size()))
                || !juce::isPositiveAndBelow(voice, static_cast<int>(state.destination->size())))
                continue;

            auto* sourceModule = state.source->at(voice);
            auto* destinationModule = state.destination->at(voice);
            if (sourceModule == nullptr || destinationModule == nullptr)
                continue;

            auto* destValue = destinationModule->params[state.destinationParamIndex];
            if (destValue == nullptr)
                continue;

            tMappingAdd_(voice_mapping,
                &sourceModule->outputs[0],
                sourceModule->uniqueID,
                destValue,
                destinationModule->uniqueID,
                destinationModule->setterFunctions[state.destinationParamIndex],
                state.destinationParamIndex,
                destinationModule,
                &leaf,
                &state.scalingValue);
        }

        return true;
    }

    bool SoundEngine::updateGraphConnection(const electrosynth::ConnectionRecord& connection)
    {
        return moduleGraph_ != nullptr && moduleGraph_->update(connection);
    }

    void SoundEngine::disconnectGraphConnection(const juce::String& connectionId)
    {
        if (moduleGraph_ == nullptr)
            return;

        const auto& connections = moduleGraph_->getConnections();
        const auto connectionIt = std::find_if(connections.begin(), connections.end(),
            [&connectionId](const auto& connection)
            {
                return connection.id == connectionId;
            });

        const auto targetConnectionExists = connectionIt != connections.end();
        const auto* targetConnection = targetConnectionExists ? &*connectionIt : nullptr;

        const bool otherConnectionStillDrivesDestination =
            targetConnection != nullptr
            && std::any_of(connections.begin(), connections.end(),
                [targetConnection](const auto& connection)
                {
                    return connection.id != targetConnection->id
                        && connection.type == targetConnection->type
                        && connection.destination.endpointId == targetConnection->destination.endpointId
                        && connection.destinationSlot == targetConnection->destinationSlot;
                });

        auto stateIt = modulationStates_.find(connectionId);
        if (stateIt != modulationStates_.end())
        {
            for (auto& mapping : stateIt->second.mapping)
            {
                if (mapping == nullptr)
                    continue;

                if (!otherConnectionStillDrivesDestination
                    && mapping->destObject != nullptr
                    && mapping->initialVal != nullptr
                    && mapping->paramID < 255)
                {
                    leaf::tMapping_setParameter(mapping->destObject, mapping->paramID, *mapping->initialVal);
                }

                auto* toFree = mapping;
                tMapping_free(&toFree);
                mapping = nullptr;
            }
            modulationStates_.erase(stateIt);
        }

        moduleGraph_->disconnect(connectionId);

#if JUCE_DEBUG
        moduleGraph_->debugPrint("ModuleGraph after disconnect: " + connectionId,
            [this](const juce::String& nodeId)
            {
                if (auto* module = getModuleByNodeId(nodeId))
                    return module->getDisplayName().isNotEmpty() ? module->getDisplayName() : nodeId;
                return nodeId;
            });
#endif
    }

    void SoundEngine::processMappings()
    {
        if (moduleGraph_ == nullptr)
            return;

        for (const auto& connection : moduleGraph_->getConnections())
        {
            if (connection.type != electrosynth::ConnectionType::Modulation)
                continue;

            auto stateIt = modulationStates_.find(connection.id);
            if (stateIt == modulationStates_.end())
                continue;

            auto& state = stateIt->second;
            state.record = connection;
            state.scalingValue.store(connection.amount, std::memory_order_relaxed);

            if (state.source == nullptr
                || state.destination == nullptr
                || state.destinationParamIndex < 0)
                continue;

            for (int voice = 0; voice < MAX_NUM_VOICES; ++voice)
            {
                auto* voice_mapping = state.mapping[voice];
                if (voice_mapping == nullptr)
                    continue;

                if (voice_mapping->initialVal == nullptr
                    || voice_mapping->destObject == nullptr
                    || voice_mapping->numUsedSources <= 0)
                    continue;

                if (voice_mapping->inSources[0] == nullptr
                    || voice_mapping->scalingValues[0] == nullptr)
                    continue;

                processMapping(voice_mapping);
            }
        }
    }

    void SoundEngine::processAudioConnections()
    {
        if (moduleGraph_ == nullptr)
            return;

        for (const auto& connection : moduleGraph_->getConnections())
        {
            if (connection.type != electrosynth::ConnectionType::Audio)
                continue;

            auto const* sourceModule = getModuleByNodeId(connection.source.nodeId);
            auto const* destModule = getModuleByNodeId(connection.destination.nodeId);

            if (sourceModule == nullptr || destModule == nullptr
            || sourceModule->procArray == nullptr
            || destModule->procArray == nullptr)
                continue;

            for (int v = 0; v < MAX_NUM_VOICES; ++v)
            {
                auto* src = sourceModule->procArray->at(v);
                auto* dst = destModule->procArray->at(v);

                if (src != nullptr && dst != nullptr)
                    dst->summedInput += src->outputs[0] * connection.amount;
            }
        }
    }

    void SoundEngine::process (juce::AudioSampleBuffer& audio_buffer, int channels, int samples, int offset)
    {
        //VITAL_ASSERT(num_samples <= output()->buffer_size);
        // benchmark();
        juce::FloatVectorOperations::disableDenormalisedNumberSupport();
        temp_voice_buffer.clear();
        for (auto& buffer : temp_fx_buffers)
            buffer.clear();
        //juce::MidiBuffer midimessages;
        int mpe = voiceHandler.mpeMode ? 1 : 0;
        int impe = 1 - mpe;
        for (int i = offset; i < samples + offset; i++)
        {
            for (int v = 0; v < voiceHandler.numVoicesActive; ++v)
            {
                float tempNote = (float) tSimplePoly_getPitch (voiceHandler.voices, (uint8_t) (v * impe));

                //added this check because if there is no active voice "getPitch" returns -1
                if (tempNote >= 0.0f)
                {
                    //freeze pitch bend data on voices where a note off has happened and we are in the release phase
                    if (tSimplePoly_isOn (voiceHandler.voices, (uint8_t) (v * impe)))
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
                        int tempNoteIntPart = (int) tempNote;
                        float tempNoteFloatPart = tempNote - (float) tempNoteIntPart;
                        //int tempPitchClassIntPart =tempNoteIntPart % 12;
                        //float dev1 = (centsDeviation[tempNoteIntPart] * (1.0f - tempNoteFloatPart));
                        //float dev2 =  (centsDeviation[(tempNoteIntPart+1)] * tempNoteFloatPart);
                        //float tunedNote = ( dev1  + dev2);
                        voiceHandler.voiceNote[v] = tempNote;
                    }
                    else //otherwise, assume octave equivalency and get offsets, then get midinote back
                    //not going to work for non-octave tunings
                    {
                        if (!isnan (tempNote) && !isinf (tempNote))
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
            {
                juce::ScopedLock sl (myCoolLock);
                auto amp_vals = MasterVoiceEnvelopeProcessor->processMasterEnvelope();

                processMappings();
                processAudioConnections();

                for (auto& modLane : modSources)
                {
                    for (auto& modulator : modLane)
                    {
                        if (modulator != nullptr)
                            modulator->tick();
                    }
                }

                int chainIndex = -1;
                for (auto& proc_chain : processors)
                {
                    chainIndex++;
                    if (proc_chain.empty())
                        continue;
                    for (auto& proc : proc_chain)
                    {
                        if (proc != nullptr)
                            proc->processBlock (temp_voice_buffer, empty);
                    }
                    // //at end of given processor chain
                    // for ( int v = 0; v < voiceHandler.numVoicesActive; ++v) {
                    //         // audio_buffer.addSample(0, i, temp_voice_buffer.getSample(v*2, 0));
                    //         // audio_buffer.addSample(1, i, temp_voice_buffer.getSample(v*2+1, 0));
                    //     // if (amp_vals->getSample(v*2,0) > 0.f) {
                    //     //     DBG(amp_vals->getSample(v*2,0));
                    //     //     DBG(temp_voice_buffer.getSample(v*2,0));
                    //     // }
                    //         audio_buffer.addSample(0, i, amp_vals->getSample(v*2, 0) * temp_voice_buffer.getSample(v*2, 0));
                    //        audio_buffer.addSample(1, i, amp_vals->getSample(v*2+1, 0) * temp_voice_buffer.getSample(v*2+1, 0));
                    // }

                    for (int v = 0; v < voiceHandler.numVoicesActive; ++v)
                    {
                        // audio_buffer.addSample(0, i, temp_voice_buffer.getSample(v*2, 0));
                        // audio_buffer.addSample(1, i, temp_voice_buffer.getSample(v*2+1, 0));
                        temp_voice_buffer.setSample (
                            v * 2,
                            0,
                            amp_vals->getSample (v * 2, 0) * temp_voice_buffer.getSample (v * 2, 0)
                            );
                        temp_voice_buffer.setSample (
                            v * 2 + 1,
                            0,
                            amp_vals->getSample (v * 2 + 1, 0) * temp_voice_buffer.getSample (v * 2 + 1, 0));
                    }
                    //writes out to fx_buffers
                    chainPostGain[chainIndex]->processBlock (temp_voice_buffer, empty);


                    temp_voice_buffer.clear();
                }
            }

            int index = 1;
            for (int v = 0; v < voiceHandler.numVoicesActive; ++v)
            {
                audio_buffer.addSample (0, i, temp_fx_buffers[0].getSample (v * 2, 0));
                audio_buffer.addSample (1, i, temp_fx_buffers[0].getSample (v * 2 + 1, 0));
            }
            std::size_t effectLaneIndex = 0;
            for (auto& fx_lane : effects)
            {
                const auto laneGain = effectLaneTransitions_[effectLaneIndex].advance();
                for (auto& fx : fx_lane)
                {
                    if (fx != nullptr) fx->processBlock (temp_fx_buffers[index], empty);
                }
                for (int v = 0; v < voiceHandler.numVoicesActive; ++v)
                {
                    audio_buffer.addSample (0, i, laneGain * temp_fx_buffers[index].getSample (v * 2, 0));
                    audio_buffer.addSample (1, i, laneGain * temp_fx_buffers[index].getSample (v * 2 + 1, 0));
                }
                index++;
                effectLaneIndex++;
            }

            for (auto& fx : temp_fx_buffers)
            {
                fx.clear();
            }
            // melatonin::printSparklin   e (*amp_vals.get,true);
        }
        // melatonin::printSparkline(audio_buffer, true);

        if (getNumActiveVoices() == 0)
        {
        }
        //   bufferDebugger->capture("main out", audio_buffer.getReadPointer(0), audio_buffer.getNumSamples(), -20.f, 20.f);
    }

    void SoundEngine::process (juce::AudioSampleBuffer& audio_buffer, juce::MidiBuffer& midi_buffer)
    {
    }

    //  void SoundEngine::correctToTime(double seconds) {
    ////    voice_handler_->correctToTime(seconds);
    ////    effect_chain_->correctToTime(seconds);
    //  }

    void SoundEngine::allSoundsOff()
    {
        //    voice_handler_->allSoundsOff();
        //    effect_chain_->hardReset();
        //    decimator_->hardReset();
    }

    void SoundEngine::allNotesOff (int sample)
    {
        //    voice_handler_->allNotesOff(sample);
    }

    void SoundEngine::allNotesOff (int sample, int channel)
    {
        //    voice_handler_->allNotesOff(channel);
    }

    void SoundEngine::allNotesOffRange (int sample, int from_channel, int to_channel)
    {
        //    voice_handler_->allNotesOffRange(sample, from_channel, to_channel);
    }

    void SoundEngine::noteOn (int note, float velocity, int sample, int channel)
    {
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
        //DBG ("noteon entered");
        int i = voiceHandler.mpeMode ? channel : 0;
        if (i < 0)
            return;
        if (!velocity)
            noteOff (note, velocity, sample, channel);
        else
        {
            int v = tSimplePoly_noteOn (voiceHandler.voices, note, velocity * 127.f);
            tStack_add(voiceHandler.voiceOrder, v);

            //DBG ("note on: " + String(tStack_get(voiceHandler.voiceOrder, 0)) + String(tStack_get(voiceHandler.voiceOrder, 1)) + String(tStack_get(voiceHandler.voiceOrder, 2)) + String(tStack_get(voiceHandler.voiceOrder, 3)) + String(tStack_get(voiceHandler.voiceOrder, 4)) + String(tStack_get(voiceHandler.voiceOrder, 5)) + String(tStack_get(voiceHandler.voiceOrder, 6)) + String(tStack_get(voiceHandler.voiceOrder, 7)));

            if (!voiceHandler.mpeMode)
                i = v;
            // DBG("note on" + String(i) + " " + String(v));

            if (v >= 0)
            {
                velocity = ((0.007685533519034f * velocity * 127.f) + 0.0239372430f);
                velocity = velocity * velocity;
                //note -= midiKeyMin;
                //note -= midiKeyMin;
                {
                    juce::ScopedLock sl (myCoolLock);
                    for (int j = 0; j < voiceHandler.eventEmitter.numListeners; j++)
                    {
                        callNoteOn (voiceHandler.eventEmitter.listeners[v][j],
                            velocity);
                    }
                }

                callNoteOn (&MasterVoiceEnvelopeProcessor->state_.params.modules[v]->header, velocity);
                voiceHandler.voiceIsSounding[v] = true;
                // float norm = key / float(mkkkidiKeyMax - midiKeyMin);
            }
        }
    }

    void SoundEngine::noteOff (int note, float velocity, int sample, int channel)
    {
        //    voice_handler_->noteOff(note, lift, sample, channel);
        //DBG ("noteoff entered");
        int i = voiceHandler.mpeMode ? channel : 0;

        if (i < 0)
            return;

        int v = tSimplePoly_markPendingNoteOff (voiceHandler.voices, note);
        //tStack_remove(voiceHandler.voiceOrder, v);

        //If stack_IsNOTEmpty
        if ((v != -1) && (tStack_getSize (tSimplePoly_getStack (voiceHandler.voices)) >= voiceHandler.numVoicesActive))
        {
            if (tSimplePoly_getVoices (voiceHandler.voices)[v][0] == -2)
            {
                tSimplePoly_deactivateVoice (voiceHandler.voices, v);
                voiceHandler.voiceIsSounding[v] = true;
                tStack_remove(voiceHandler.voiceOrder, v);
                //DBG ("off: " + String(tStack_get(voiceHandler.voiceOrder, 0)) + String(tStack_get(voiceHandler.voiceOrder, 1)) + String(tStack_get(voiceHandler.voiceOrder, 2)) + String(tStack_get(voiceHandler.voiceOrder, 3)) + String(tStack_get(voiceHandler.voiceOrder, 4)) + String(tStack_get(voiceHandler.voiceOrder, 5)) + String(tStack_get(voiceHandler.voiceOrder, 6)) + String(tStack_get(voiceHandler.voiceOrder, 7)));


            }
            //DBG ("noteoff stacknot empty" + String (i) + " " + String (v));
            //tStack_remove(voiceHandler.voiceOrder, v);
            //DBG ("off: " + String(tStack_get(voiceHandler.voiceOrder, 0)) + String(tStack_get(voiceHandler.voiceOrder, 1)) + String(tStack_get(voiceHandler.voiceOrder, 2)) + String(tStack_get(voiceHandler.voiceOrder, 3)) + String(tStack_get(voiceHandler.voiceOrder, 4)) + String(tStack_get(voiceHandler.voiceOrder, 5)) + String(tStack_get(voiceHandler.voiceOrder, 6)) + String(tStack_get(voiceHandler.voiceOrder, 7)));
            return;
        }
        if (!voiceHandler.mpeMode)
            i = v; //not sure if this is right -JS
        //DBG ("noteoff" + String (i) + " " + String (v));
        //DBG ("noteoff " + String(v));
        //tStack_remove(voiceHandler.voiceOrder, v);
        //DBG ("off: " + String(tStack_get(voiceHandler.voiceOrder, 0)) + String(tStack_get(voiceHandler.voiceOrder, 1)) + String(tStack_get(voiceHandler.voiceOrder, 2)) + String(tStack_get(voiceHandler.voiceOrder, 3)) + String(tStack_get(voiceHandler.voiceOrder, 4)) + String(tStack_get(voiceHandler.voiceOrder, 5)) + String(tStack_get(voiceHandler.voiceOrder, 6)) + String(tStack_get(voiceHandler.voiceOrder, 7)));


        if (v >= 0)
        {
            //note -= midiKeyMin;
            for (uint8_t j = 0; j < voiceHandler.eventEmitter.numListeners; j++)
            {
                callNoteOn (voiceHandler.eventEmitter.listeners[v][j],
                    -1);
            }

            callNoteOn (&MasterVoiceEnvelopeProcessor->state_.params.modules[v]->header, -1);
            voiceHandler.voiceIsSounding[v] = true;
            // float norm = key / float(mkkkidiKeyMax - midiKeyMin);
        }
    }

    void SoundEngine::setModWheel (float value, int channel)
    {
        //    voice_handler_->setModWheel(value, channel);
    }

    void SoundEngine::setModWheelAllChannels (float value)
    {
        //    voice_handler_->setModWheelAllChannels(value);
    }

    void SoundEngine::setPitchWheel (float value, int channel)
    {
        //    voice_handler_->setPitchWheel(value, channel);
    }

    void SoundEngine::setZonedPitchWheel (float value, int from_channel, int to_channel)
    {
        //    voice_handler_->setZonedPitchWheel(value, from_channel, to_channel);
    }

    void SoundEngine::setAftertouch (float note, float value, int sample, int channel)
    {
        //    voice_handler_->setAftertouch(note, value, sample, channel);
    }

    void SoundEngine::setChannelAftertouch (int channel, float value, int sample)
    {
        //    voice_handler_->setChannelAftertouch(channel, value, sample);
    }

    void SoundEngine::setChannelRangeAftertouch (int from_channel, int to_channel, float value, int sample)
    {
        //    voice_handler_->setChannelRangeAftertouch(from_channel, to_channel, value, sample);
    }

    void SoundEngine::setChannelSlide (int channel, float value, int sample)
    {
        //    voice_handler_->setChannelSlide(channel, value, sample);
    }

    void SoundEngine::setChannelRangeSlide (int from_channel, int to_channel, float value, int sample)
    {
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

    void SoundEngine::sustainOn (int channel)
    {
        //    voice_handler_->sustainOn(channel);
    }

    void SoundEngine::sustainOff (int sample, int channel)
    {
        //    voice_handler_->sustainOff(sample, channel);
    }

    void SoundEngine::sostenutoOn (int channel)
    {
        //    voice_handler_->sostenutoOn(channel);
    }

    void SoundEngine::sostenutoOff (int sample, int channel)
    {
        //    voice_handler_->sostenutoOff(sample, channel);
    }

    void SoundEngine::sustainOnRange (int from_channel, int to_channel)
    {
        //    voice_handler_->sustainOnRange(from_channel, to_channel);
    }

    void SoundEngine::sustainOffRange (int sample, int from_channel, int to_channel)
    {
        //    voice_handler_->sustainOffRange(sample, from_channel, to_channel);
    }

    void SoundEngine::sostenutoOnRange (int from_channel, int to_channel)
    {
        //    voice_handler_->sostenutoOnRange(from_channel, to_channel);
    }

    void SoundEngine::sostenutoOffRange (int sample, int from_channel, int to_channel)
    {
        //    voice_handler_->sostenutoOffRange(sample, from_channel, to_channel);
    }

    std::array<ModuleHeader*, MAX_NUM_VOICES>* SoundEngine::getLEAFProcessor (const std::string& proc_string) {

        auto findProcessor = [&proc_string](auto& lanes) -> ProcessorBase* {
            for (auto& lane : lanes) {
                for (auto& processor : lane) {
                    if (processor != nullptr && processor->name == juce::String(proc_string)) {
                        return processor.get();
                    }
                }
            }
            return nullptr;
        };

        if (auto* processor = findProcessor(processors))
            return processor->procArray;

        if (auto* effect = findProcessor(effects))
            return effect->procArray;

        if (proc_string == "VCA" || (MasterVoiceEnvelopeProcessor != nullptr
                && MasterVoiceEnvelopeProcessor->name == juce::String (proc_string)))
            return MasterVoiceEnvelopeProcessor->procArray;

        return nullptr;
    }

    std::array<ModuleHeader*, MAX_NUM_VOICES>* SoundEngine::getLEAFProcessorModulator (const std::string& proc_string)
    {
        // Use find_if to search the outermost vector
        auto outerIt = std::find_if (modSources.begin(), modSources.end(), [&] (const auto& innerVec) {
            // Use find_if on the inner vector to look for the processor with the target name
            auto innerIt = std::find_if (innerVec.begin(), innerVec.end(), [&] (const auto& processor) {
                return processor != nullptr && processor->name == juce::String (proc_string);
            });

            // Return true if the processor was found in this inner vector
            return innerIt != innerVec.end();
        });

        if (outerIt != modSources.end())
        {
            auto innerIt = std::find_if (outerIt->begin(), outerIt->end(), [&] (const auto& processor) {
                return processor != nullptr && processor->name == juce::String (proc_string);
            });

            // Here you can cast the processor to leaf::Processor* if needed
            return (innerIt->get()->procArray);
        }
        if (proc_string == "VCA")
        {
            return MasterVoiceEnvelopeProcessor->procArray;
        }
        jassertfalse;
        return nullptr;
    }

    std::pair<std::array<ModuleHeader*, MAX_NUM_VOICES>*, int> SoundEngine::getParameterInfo (const std::string& value)
    {
        std::stringstream ss (value);
        std::string proc_string;
        std::getline (ss, proc_string, '_');

        auto proc = getLEAFProcessor (proc_string);
        if (proc == nullptr || proc->at (0) == nullptr)
            return { nullptr, -1 };

        int procID = proc->at (0)->moduleType;
        if (procID < 0 || procID >= static_cast<int> (paramsAllArray.size()))
            return { nullptr, -1 };

        std::string param_string;
        std::getline (ss, param_string, '_');
        int index = -1;
        auto it = std::find (paramsAllArray[procID].cbegin(), paramsAllArray[procID].cend(), param_string);
        if (it != paramsAllArray[procID].cend())
        {
            // Calculate the index
            index = static_cast<int> (std::distance (paramsAllArray[procID].cbegin(), it));
        }

        return { proc, index };
    }

    //  leaf::tProcessor * SoundEngine::getLEAFProcessor (const std::string& proc_string) {
    //
    //  }

    ProcessorBase* SoundEngine::getProcessorFromUUID (int uuid)
    {
    }

    ModulatorBase* SoundEngine::getModulatorFromUUID (int uuid)
    {
    }

    std::array<ModuleHeader*, MAX_NUM_VOICES>* SoundEngine::getLeafProcessorFromUUID (int uuid)
    {
    }

    void SoundEngine::connectMapping (const electrosynth::mapping_change& change)
    {
        if (moduleGraph_ == nullptr || change.connection == nullptr)
            return;

        electrosynth::ConnectionRecord record {
            .id = juce::String (change.connection->uuid),
            .type = electrosynth::ConnectionType::Modulation,
            .source {
                .type = electrosynth::ConnectionType::Modulation,
                .nodeId = juce::String (change.connection->source_name),
                .endpointId = juce::String (change.connection->source_name),
                .direction = electrosynth::EndpointDirection::Source
            },
            .destination {
                .type = electrosynth::ConnectionType::Modulation,
                .nodeId = juce::String (change.connection->destination_name),
                .endpointId = juce::String (change.connection->destination_name),
                .direction = electrosynth::EndpointDirection::Destination
            },
            .destinationSlot = change.connection->destination_slot,
            .amount = change.connection->getCurrentBaseValue(),
            .bipolar = change.connection->isBipolar(),
            .bypass = change.connection->isBypass(),
            .stereo = change.connection->isStereo()
        };

        if (!moduleGraph_->connect(record))
            return;

        auto& state = modulationStates_[record.id];
        state.record = record;
        state.scalingValue.store(record.amount, std::memory_order_relaxed);
        state.source = change._source;
        state.destination = change._dest;
        state.destinationParamIndex = change.dest_param_index;

        if (state.source == nullptr || state.destination == nullptr || state.destinationParamIndex < 0)
        {
            moduleGraph_->disconnect(record.id);
            modulationStates_.erase(record.id);
            return;
        }

        for (int voice = 0; voice < MAX_NUM_VOICES; ++voice)
        {
            if (state.mapping[voice] == nullptr)
                tMapping_init(&state.mapping[voice], &leaf);

            auto* voice_mapping = state.mapping[voice];
            if (voice_mapping == nullptr
                || !juce::isPositiveAndBelow(voice, static_cast<int>(state.source->size()))
                || !juce::isPositiveAndBelow(voice, static_cast<int>(state.destination->size())))
                continue;

            auto* sourceModule = state.source->at(voice);
            auto* destinationModule = state.destination->at(voice);
            if (sourceModule == nullptr || destinationModule == nullptr)
                continue;

            auto* destValue = destinationModule->params[state.destinationParamIndex];
            if (destValue == nullptr)
                continue;

                tMappingAdd_ (voice_mapping,
                &sourceModule->outputs[0],
                sourceModule->uniqueID,
                destValue,
                destinationModule->uniqueID,
                destinationModule->setterFunctions[state.destinationParamIndex],
                state.destinationParamIndex,
                destinationModule,
                &leaf,
                &state.scalingValue);
        }

        DBG ("added new modulation");
    }

    //returns true if the mapping should be completely removd from process mappings
    void SoundEngine::disconnectMapping (const electrosynth::mapping_change& change)
    {
        if (change.connection == nullptr)
            return;

        const auto connectionId = juce::String (change.connection->uuid);
        if (moduleGraph_ != nullptr)
            moduleGraph_->disconnect(connectionId);

        auto stateIt = modulationStates_.find(connectionId);
        if (stateIt == modulationStates_.end())
            return;

        for (auto& mapping : stateIt->second.mapping)
        {
            if (mapping == nullptr)
                continue;

            auto* toFree = mapping;
            tMapping_free(&toFree);
            mapping = nullptr;
        }

        modulationStates_.erase(stateIt);
    }
} // namespace vital
