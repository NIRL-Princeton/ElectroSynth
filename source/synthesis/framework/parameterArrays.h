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
            "oscType",
            "syncToNoteOn"},

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
            "resonance", // FiltResonance
            "filterType", // FiltType
            "keyfollow", // FiltKeyfollow
            "gain", // FiltGain
            "filtMix", // FiltAudioInput
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
            "mix",
           "" // OscNumParams (typically represents the count, no corresponding parameter)
        },

        // delay
        {
            "eventWatch",//DelayEventWatchFlag,
            "delayTime",//     DelayTime,
            "delayHP",//    DelayHPCutoff,
            "delayLP",//    DelayLPCutoff,
            "delayFB",//    DelayFeedback,
            "delayGain",//    DelayGain,
            "delayMix",
            ""    //DelayNumParams

        },

        // noise
        {
            "eventWatch",
            "gain",
            "tilt",
            "peakGain",
            "peakFreq",
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
            "rateHz",
            "energy"
        },

        // sine
        {
            "eventWatch",
            "harmonic",
            "pitchFine",
            "pitch",
            "amp",
            "gain"
            ""
        },

        // sample and hold
        {
            "eventWatch",
            "threshold",
            "frequency",
            "durRand",
            "gain",
            "mix",
               ""
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
