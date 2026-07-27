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
const std::array<std::vector<std::string>, 11> paramsAllArray =
{
{
    // oscillator
    {
        "eventWatch",
        "audioIn",
        "midiPitch", // OscMidiPitch

        "harmonic", // OscHarmonic
        "pitch", // OscPitchOffset
        "pitchfine", // OscPitchFine
        "freqoffset", // OscFreqOffset
        "shape", // OscShapeParam
        "amp", // OscAmpParam
        "glide", // OscGlide
        "harmonicStepped", // OscSteppedHarmonic
        "pitchStepped", // OscSteppedPitch
        "", // OscSyncMode (undefined / not modulatable )
        "", // OscSyncIn (undefined / not modulaatblae )
        "oscType", // OscType
        "" // OscNumParams (typically represents the count, no corresponding parameter)
    },

    // lfo
    {
        "eventWatch",
        "audioIn",
        "rate",
        "shape",
        "phase",
        "",
        ""},

    // envelope
    {
        "eventWatch", // EnvEventWatchFlag
        "audioIn",
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
        "audioIn",
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
        "audioIn",
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
        "audioIn",
       "inputGain", // OscHarmonic
       "offset", // OscPitchOffset
       "shape", // OscPitchFine
       "outputGain", // OscFreqOffset
       "" // OscNumParams (typically represents the count, no corresponding parameter)
    },

    // delay
    {
        "eventWatch",
        "audioIn",
        "delayTime",
        "gain",
        "",
        ""
    },

    // noise
    {
        "eventWatch",
        "audioIn",
        "gain",
        "tilt",
        "peakAmt",
        "peakFreq",
        "peakBandwidth"
    },

    // simpleNoise
    {
        "eventWatch",
        "audioIn",
        "gain"
    },

    // perlinNoise
    {
        "eventWatch",
        "audioIn",
        "gain",
        "rate",
        "energy"
    }
}

};

#endif //ELECTORSYNTH_PARAMETERARRAYS_H
