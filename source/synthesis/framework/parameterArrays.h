//
// Created by Davis Polito on 1/20/25.
//

#ifndef ELECTORSYNTH_PARAMETERARRAYS_H
#define ELECTORSYNTH_PARAMETERARRAYS_H
//only include this in the sound_engine.cpp file so that it only creates one instance
#include <array>

const std::array<std::vector<std::string>,3> paramsAllArray =
    {
    {
            {
                "", // OscMidiPitch
                "harmonic", // OscHarmonic
                "pitch", // OscPitchOffset
                "pitchfine", // OscPitchFine
                "freqoffset", // OscFreqOffset
                "shape", // OscShapeParam
                "amp", // OscAmpParam
                "glide", // OscGlide
                "harmonicStepped", // OscStepped
                "", // OscSyncMode (undefined in your parameters)
                "", // OscSyncIn (undefined in your parameters)
                "oscType", // OscType
                "" // OscNumParams (typically represents the count, no corresponding parameter)
            },
            {},
            {},
        }

    };

#endif //ELECTORSYNTH_PARAMETERARRAYS_H
