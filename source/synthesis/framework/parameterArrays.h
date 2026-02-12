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
const std::array<std::vector<std::string>, 7> paramsAllArray =
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
        {"eventWatch"},

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
        }
    }

};

#endif //ELECTORSYNTH_PARAMETERARRAYS_H
