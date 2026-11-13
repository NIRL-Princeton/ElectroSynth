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

#include <unordered_set>

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
        for (auto& laneInput : laneSummedInputs)
        {
            laneInput.setSize(MAX_NUM_VOICES * 2, 1);
            laneInput.clear();
        }
        for (auto& laneBuffer : laneProcessingBuffers)
        {
            laneBuffer.setSize(MAX_NUM_VOICES * 2, 1);
            laneBuffer.clear();
        }
        for (auto& chainPostGainBuffer : chainPostGainBuffers)
        {
            chainPostGainBuffer.setSize(MAX_NUM_VOICES * 2, 1);
            chainPostGainBuffer.clear();
        }
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

    void SoundEngine::processMappings(const RuntimeTopologySnapshot& snapshot)
    {
        for (const auto& runtimeConnection : snapshot.modulationConnections)
        {
            if (runtimeConnection.state == nullptr)
                continue;

            auto& moduleRuntime = *runtimeConnection.state;
            if (moduleRuntime.source == nullptr
                || moduleRuntime.destination == nullptr
                || moduleRuntime.destinationParamIndex < 0)
                continue;

            for (int voice = 0; voice < voiceHandler.numVoicesActive; ++voice)
            {
                auto* voice_mapping = moduleRuntime.mapping[voice];
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

    void SoundEngine::processAudioConnections(juce::AudioBuffer<float>& masterEnvelope,
                                              const RuntimeTopologySnapshot& snapshot)
    {
        for (const auto& connection : snapshot.audioConnections)
        {
            if (connection.sourceModule == nullptr || connection.destinationModule == nullptr)
                continue;

            if (connection.sourceModule->procArray == nullptr
                || connection.destinationModule->procArray == nullptr)
                continue;

            for (int v = 0; v < voiceHandler.numVoicesActive; ++v)
            {
                auto* src = connection.sourceModule->procArray->at(v);
                auto* dst = connection.destinationModule->procArray->at(v);
                if (src == nullptr || dst == nullptr)
                    continue;

                const auto envValue = masterEnvelope.getSample (v * 2, 0);
                const auto sourceOut = connection.sourceIsChainExit ? src->outputs[0].load() * envValue : src->outputs[0].load(); // intervene with the master envelope baby !
                dst->inputs[0] = dst->inputs[0] + sourceOut * connection.amount;
                dst->inputs[0] = LEAF_clip(-1.f, dst->inputs[0], 1.f); // might get aliasing... could replace with soft clip.
            }
        }
    }



    static void tickAllModules(const std::vector<ModuleBase*>& modules) noexcept
    {
        for  (auto* module : modules) if (module) module->tick();
    }

    // get rid of this ??
    void SoundEngine::mixChainOutputsWithMasterEnvelope(juce::AudioBuffer<float>& masterEnvelope,
                                                        const RuntimeTopologySnapshot& snapshot) const noexcept
    {
        for (auto* module : snapshot.chainExitModules)
        {
            if (module == nullptr || module->procArray == nullptr)
                continue;

            for (int v = 0; v < voiceHandler.numVoicesActive; ++v)
            {
                auto* header = (*module->procArray)[v];
                if (header == nullptr)
                    continue;

                const float envL = masterEnvelope.getSample(v * 2, 0);
                const float envR = masterEnvelope.getSample(v * 2 + 1, 0);

                // If mono per-voice, pick one envelope value or decide a policy here.
                header->outputs[0] = header->outputs[0] * envL;
            }
        }
    }

    void SoundEngine::mixTerminalModulesToOutput(juce::AudioSampleBuffer& audio_buffer,
                                                 juce::AudioBuffer<float>& masterEnvelope,
                                                 const RuntimeTopologySnapshot& snapshot,
                                                 int i,
                                                 bool mixWithMasterEnvelope) const
    {
        for (const auto& terminalModule : snapshot.terminalAudioModules)
        {
            auto* module = terminalModule.module;
            if (module == nullptr || module->procArray == nullptr)
                continue;

            const auto* procArray = module->procArray;
            for (int v = 0; v < voiceHandler.numVoicesActive; ++v)
            {
                auto* header = (*procArray)[v];
                if (header == nullptr)
                    continue;

                const float env = terminalModule.isChainExit ? masterEnvelope.getSample(v * 2, 0) : 1.0f;
                audio_buffer.addSample (
                    0,
                    i,
                    mixWithMasterEnvelope ? env : 1.0f * header->outputs[0] * env);
                audio_buffer.addSample (
                    1,
                    i,
                    mixWithMasterEnvelope ? env : 1.0f * header->outputs[0] * env);

            }
        }
    }

    void SoundEngine::debugPrintTerminalAudioModules(const juce::String& header) const
    {
#if JUCE_DEBUG
        juce::String out;
        if (header.isNotEmpty())
            out << header << "\n";

        out << "Terminal audio modules: "
            << juce::String(terminalAudioModules_.size()) << "\n";

        if (terminalAudioModules_.empty())
        {
            out << "    <none>\n";
            DBG(out);
            return;
        }

        for (std::size_t index = 0; index < terminalAudioModules_.size(); ++index)
        {
            const auto* module = terminalAudioModules_[index];
            if (module == nullptr)
            {
                out << "    [" << juce::String(index) << "] <null>\n";
                continue;
            }

            out << "    [" << juce::String(index) << "] "
                << module->getNodeId()
                << " display=\"" << module->getDisplayName() << "\""
                << " ptr=" << juce::String::toHexString(static_cast<juce::uint64>(
                    reinterpret_cast<juce::pointer_sized_int>(module)));

            if (module->procArray != nullptr)
                out << " voices=" << juce::String(module->procArray->size());

            out << "\n";
        }

        DBG(out);
#endif
    }

    void SoundEngine::debugPrintChainExitModules() const
    {
#if JUCE_DEBUG
        juce::String out;

        out << "Num chain exit modules: "
            << juce::String(static_cast<int>(chainExitModules_.size())) << "\n";

        if (chainExitModules_.empty())
        {
            out << "    <none>\n";
            DBG(out);
            return;
        }

        int index = 0;
        for (const auto* module : chainExitModules_)
        {
            if (module == nullptr)
            {
                out << "    [" << juce::String(index) << "] <null>\n";
                ++index;
                continue;
            }

            out << "    display=\"" << module->getDisplayName() << "\"\n";
            ++index;
        }

        DBG(out);
#endif
    }


    static void flushLaneInputToBuffer(juce::AudioBuffer<float>& laneInput, juce::AudioBuffer<float>& laneBuffer)
    {
        if (laneInput.getNumChannels() == 0 || laneBuffer.getNumChannels() == 0)
            return;

        const int numVoices = juce::jmin(laneInput.getNumChannels(), laneBuffer.getNumChannels()) / 2;
        for (int v = 0; v < numVoices; ++v)
        {
            const int left = v * 2;
            const int right = left + 1;
            laneBuffer.addSample(left, 0, laneInput.getSample(left, 0));
            laneBuffer.addSample(right, 0, laneInput.getSample(right, 0));
        }

        laneInput.clear();
    }

    void SoundEngine::process (juce::AudioSampleBuffer& audio_buffer, int channels, int samples, int offset)
    {
        //VITAL_ASSERT(num_samples <= output()->buffer_size);
        // benchmark();
        juce::FloatVectorOperations::disableDenormalisedNumberSupport();

        // Create / load runtime graph at block level instead of every sample.
        static const RuntimeTopologySnapshot kEmptySnapshot;
        const auto runtimeSnapshot = getRuntimeTopologySnapshot();
        const RuntimeTopologySnapshot& snapshot = runtimeSnapshot != nullptr ? *runtimeSnapshot : kEmptySnapshot;
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
                const auto masterEnvelope = MasterVoiceEnvelopeProcessor->processMasterEnvelope();

                processMappings(snapshot); // process modulations !! :)

                processAudioConnections(*masterEnvelope, snapshot); // process all audio connections (e.g. route their audio to and fro) - robert frost

                tickAllModules(snapshot.modulesToTick); // tick all the modules, one by one, indiscriminately (cold)

                mixTerminalModulesToOutput(audio_buffer, *masterEnvelope, snapshot, i); // get actual outputs from graph (terminal modules) and fill JUCE output buffer

                // Mike notes:
                // 1. "snapshot" paradigm is to make audio thread more efficient and deterministic as it should be.
                // 2. eventually worth reworking modulegraph / topology paradigm to be "agnostic" of ElectroSynth so that we can reuse for other projects
                // 3. may be some non-thread safe stuff with changes to modulegraph that could cause clicks in software side when a preset changes.
                //    when snapshot is taken, we need to make sure that all modules are in a consistent state.
            }
        }

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
                //velocity = ((0.007685533519034f * velocity * 127.f) + 0.0239372430f);
                //velocity = velocity * velocity;
                //now doing this in the envelope
                //note -= midiKeyMin;
                //note -= midiKeyMin;
                {
                    juce::ScopedLock sl (myCoolLock);
                    for (int j = 0; j < voiceHandler.eventEmitter.numListeners; j++)
                    {
                        callNoteOn (voiceHandler.eventEmitter.listeners[v][j],
                            velocity);
                        //printf("On On 1.  ");
                    }
                }

                callNoteOn (&MasterVoiceEnvelopeProcessor->state_.params.modules[v]->header, velocity);
                voiceHandler.voiceIsSounding[v] = true;
                //printf("On On 2.  ");
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
                //printf("Off On 1.  ");
            }

            callNoteOn (&MasterVoiceEnvelopeProcessor->state_.params.modules[v]->header, -1);
            voiceHandler.voiceIsSounding[v] = true;
            //printf("Off On 2.");
            // float norm = key / float(mkkkidiKeyMax - midiKeyMin);
        }
        //printf("\n");
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

} // namespace vital
