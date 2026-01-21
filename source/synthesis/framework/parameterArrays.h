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
        {
<<<<<<< HEAD
            "eventWatch", // OscMidiPitch
=======
            "eventWatch",
            "midiPitch", // OscMidiPitch
>>>>>>> 1400e5cb29bafad98d3f2112652dd0ef12f375d7
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
<<<<<<< HEAD
        },//oscillator
        {}, //lfo
        {}, //env
=======
        },
        {
            "eventWatch"
        "rate","shape","phase","",""}, //lfo
        {"eventWatch"}, //env
>>>>>>> 1400e5cb29bafad98d3f2112652dd0ef12f375d7
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
        {}, //string
        {
            "eventWatch", //
           "inputGain", // OscHarmonic
           "offset", // OscPitchOffset
           "shape", // OscPitchFine
           "outputGain", // OscFreqOffset
           "" // OscNumParams (typically represents the count, no corresponding parameter)
        }, //softclip
    }

};

#endif //ELECTORSYNTH_PARAMETERARRAYS_H
