//
// Created by Davis Polito on 1/20/25.
//

#ifndef ELECTORSYNTH_PARAMETERARRAYS_H
#define ELECTORSYNTH_PARAMETERARRAYS_H
//only include this in the sound_engine.cpp file so that it only creates one instance
#include <array>
// typedef enum {
//     ModuleTypeOscModule,
//     ModuleTypeLFOModule,
//     ModuleTypeEnvModule,
//     ModuleTypeFilterModule,
//     ModuleTypeStringModule,
//     ModuleTypeVCAModule,
//
// } ModuleType;
// Keep this array in exactly the same order as ModuleType in defs.h.
const std::array<std::vector<std::string>, 12> paramsAllArray =
{
    {
        // oscillator
        {
            "eventWatch",
            "midiPitch", // OscMidiPitch

            "harmonic", // OscHarmonic
            "pitch", // OscPitchOffset
            "pitchfine", // OscPitchFine
            "freqoffset", // OscFreqOffset
            "shape", // OscShapeParam
            "amp", // OscAmpParam
            "glide", // OscGlide
            "harmonicStepped", // OscStepped
            "", // OscSyncMode (undefined / not modulatable )
            "", // OscSyncIn (undefined / not modulaatblae )
            "oscType", // OscType
            "portaType" // OscPortaType (always or not always)
            "" // OscNumParams (typically represents the count, no corresponding parameter)
        },

        // lfo
        {
            "eventWatch",
            "rate",
            "shape",
            "phase",
            "",
            ""},

        // envelope
        {
            "eventWatch", // EnvEventWatchFlag
            "attack",     // EnvAttack
            "decay",      // EnvDecay
            "sustain",    // EnvSustain
            "release",    // EnvRelease
            "leak",       // EnvLeak
            "shape",      // EnvShape
            "velocity"    // EnvVelocitySense
        },

        // filter
        {
            "eventWatch", // FiltEventWatchFlag
            "midiPitch", // FiltMidiPitch
            "cutoff", // FiltCutoff
            "amp", // FiltGain
            "resonance", // FiltResonance
            "keyfollow", // FiltKeyfollow
            "filterType", // FiltType
            "audioInput", // FiltAudioInput
            "" // FiltNumParams (placeholder)
        },

        // string
        {
            "eventWatch",    // StringEventWatchFlag
            "oversample",    // StringOversample
            "freq",          // StringFreq
            "waveLength",    // StringWaveLength
            "dampFreq",      // StringDampFreq
            "decay",         // StringDecay
            "targetLevel",   // StringTargetLevel
            "levelSmooth",   // StringLevelSmooth
            "levelStrength", // StringLevelStrength
            "pickupPoint",   // StringPickupPoint
            "levelMode",     // StringLevelMode
            "rippleGain",    // StringRippleGain
            "rippleDelay",   // StringRippleDelay
            "pluckPosition"  // StringPluckPosition
        },

        // VCA
        {},

        // softclip
        {
            "eventWatch", //
           "inputGain", // OscHarmonic
           "offset", // OscPitchOffset
           "shape", // OscPitchFine
           "outputGain", // OscFreqOffset
           "" // OscNumParams (typically represents the count, no corresponding parameter)
        },

        // delay
        {
            "eventWatch",
            "delayTime",
            "gain",
            "",
            ""
        },

        // noise
        {
            "eventWatch",
            "gain",
            "tilt",
            "peakAmt",
            "peakFreq",
            "peakBandwidth"
        },

        // simpleNoise
        {
            "eventWatch",
            "gain"
        },

        // perlinNoise
        {
            "eventWatch",
            "gain",
            "rate",
            "energy"
        },

        // sine
        {
            "eventWatch",
            "pitch",
            "gain"
        },
    }

};

#endif //ELECTORSYNTH_PARAMETERARRAYS_H
