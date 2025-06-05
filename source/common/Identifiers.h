//
// Created by Davis Polito on 12/8/23.
//

#ifndef BITKLAVIER2_IDENTIFIERS_H
#define BITKLAVIER2_IDENTIFIERS_H
#pragma once
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace IDs
{
#define DECLARE_ID(name) const juce::Identifier name (#name);

    DECLARE_ID (GALLERY)
    DECLARE_ID (CONTROLS)
    DECLARE_ID (INPUT)
    DECLARE_ID (OUTPUT)
    DECLARE_ID (PIANO)
    DECLARE_ID (MODULE)
    DECLARE_ID (MODULATOR)
    DECLARE_ID (PREPARATIONS)
    DECLARE_ID(MODULATION)

    DECLARE_ID (name)

    DECLARE_ID (PREPARATION)
    DECLARE_ID (id)
    DECLARE_ID (type)
    DECLARE_ID (x)
    DECLARE_ID (y)
    DECLARE_ID (height)
    DECLARE_ID (width)
    DECLARE_ID (numIns)
    DECLARE_ID (numOuts)
    DECLARE_ID (nodeID)

    DECLARE_ID (CONNECTION)
    DECLARE_ID (src)
    DECLARE_ID (dest)
    DECLARE_ID (srcIdx)
    DECLARE_ID (destIdx)
    DECLARE_ID(isIn)
    DECLARE_ID (PORT)
    DECLARE_ID (chIdx)
    DECLARE_ID (modAmt)
    DECLARE_ID (isBipolar)
  //type
  //


    DECLARE_ID (assignment)

    DECLARE_ID (uuid)
    DECLARE_ID (midiInput)
    DECLARE_ID (midiDeviceId)
    DECLARE_ID(active)
    DECLARE_ID(midiPrefs)
    DECLARE_ID(SOUNDMODULE)

}

#undef DECLARE_ID
    inline juce::ValueTree createUuidProperty (juce::ValueTree& v)
    {
        if (! v.hasProperty (IDs::uuid))
            v.setProperty (IDs::uuid, juce::Uuid().toString(), nullptr);

        return v;
    }


#endif //BITKLAVIER2_IDENTIFIERS_H
