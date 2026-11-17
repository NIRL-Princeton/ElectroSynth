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
const std::array<std::vector<std::string>, 15> paramsAllArray =
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
            "oscType", // OscType
            "gain", // OscAmpParam
            "glide", // OscGlide
            "harmonicStepped", // OscStepped
            "pitchStepped", // OscSyncMode (undefined / not modulatable )
            "syncMode", // OscSyncIn (undefined / not modulatable )
            "syncIn", // OscPortaType (always or not always)
            "amp",
            "octave",
            "" // OscNumParams (typically represents the count, no corresponding parameter)
        },

        // lfo
        {
            "eventWatch",
            "rate",
            "shape",
            "phase",
            "type",
            "syncNoteOn"},

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
            "gain", // FiltGain
            "resonance", // FiltResonance
            "keyfollow", // FiltKeyfollow
            "filterType", // FiltType
            "audioInput", // FiltAudioInput
            "" // FiltNumParams (placeholder)
        },

        // string
        {
            "eventWatch",    // StringEventWatchFlag
            "dampFreq",    // StringOversample
            "freqOffset",          // StringMIDIPitch
            "decay",      // StringDampFreq
            "targetLevel",   // StringTargetLevel
            "levelSmooth",   // StringLevelSmooth
            "levelStrength", // StringLevelStrength
            "pickupPoint",   // StringPickupPoint
            "levelMode",     // StringLevelMode
            "rippleGain",    // StringRippleGain
            "rippleDelay",   // StringRippleDelay
            "pluckPosition",  // StringPluckPosition
            "octave",
            ""
        },

        // VCA
        {
            "eventWatch",    //
            "CVOffset",
            "amp",
            "gain",
            "routing",
            "audioInput",
            ""
        },
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
            ""
        },

        // noise
        {
            "eventWatch",
            "gain",
            "tilt",
            "peakAmt",
            "freqKnob",
            "peakBandwidth",
            "keyFollow",
            "glide",
            "portaType",
            "midiPitch"
        },

        // simpleNoise
        {
            "eventWatch",
            "gain"
        },

        // perlinNoise
        {
            "eventWatch",
            "amp",
            "rate",
            "energy"
        },

        // sine
        {
            "eventWatch",
            "pitch",
            "gain"
        },

        // sample and hold
        {
            "eventWatch",
            "threshold",
            "frequency",
            "durRand",
            "gain",
            "mix",
               "harmonic",
        },

        // simple envelope
        {
            "eventWatch", // EnvEventWatchFlag
            "attack",     // EnvAttack
            "decay",      // EnvDecay
            "sustain",    // EnvSustain
            "release",    // EnvRelease
            "leak",       // EnvLeak
            "velocity"    // EnvVelocitySense
        },
        // control input (knobs, cc's, etc)
            {
            "eventWatch", // ControlEventWatchFlag
            "ccParam",     // ControlCCParam
            "ccValue"      // ControlCCValue
        },

    }

};

#endif //ELECTORSYNTH_PARAMETERARRAYS_H
