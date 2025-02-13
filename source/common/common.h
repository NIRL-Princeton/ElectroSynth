//
// Created by Davis Polito on 11/8/23.
//
#pragma once

// Debugging.
#if DEBUG
    #include <cassert>
    #define _ASSERT(x) assert(x)
#else
    #define _ASSERT(x) ((void)0)
#endif // DEBUG

#define UNUSED(x) ((void)x)

#if !defined(force_inline)
    #if defined (_MSC_VER)
        #define force_inline __forceinline
        #define vector_call __vectorcall
    #else
        #define force_inline inline __attribute__((always_inline))
        #define vector_call
    #endif
#endif


namespace electrosynth {



    constexpr float kPi = 3.1415926535897932384626433832795f;
    constexpr float kSqrt2 = 1.414213562373095048801688724209698f;
    constexpr float kEpsilon = 1e-16f;
    constexpr int kMaxBufferSize = 128;
    constexpr int kMaxOversample = 8;
    constexpr int kDefaultSampleRate = 44100;
    constexpr float kMinNyquistMult = 0.45351473923f;
    constexpr int kMaxSampleRate = 192000;
    constexpr int kMidiSize = 128;
    constexpr int kMidiTrackCenter = 60;

    constexpr float kMidi0Frequency = 8.1757989156f;
    constexpr float kDbfsIncrease = 6.0f;
    constexpr int kDegreesPerCycle = 360;
    constexpr int kMsPerSec = 1000;
    constexpr int kNotesPerOctave = 12;
    constexpr int kCentsPerNote = 100;
    constexpr int kCentsPerOctave = kNotesPerOctave * kCentsPerNote;

    constexpr int kMaxModulationConnections = 64;
    constexpr int kPpq = 960; // Pulses per quarter note.
    constexpr float kVoiceKillTime = 0.05f;
    constexpr int kNumChannels = 2;
    constexpr int kNumMidiChannels = 16;
    constexpr int kFirstMidiChannel = 0;
    constexpr int kLastMidiChannel = kNumMidiChannels - 1;

} // namespace electrosynth
