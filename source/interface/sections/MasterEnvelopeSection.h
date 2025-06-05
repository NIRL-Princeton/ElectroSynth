//
// Created by Davis Polito on 6/5/25.
//

#ifndef MASTERENVELOPESECTION_H
#define MASTERENVELOPESECTION_H
#include "synth_section.h"

struct SynthGuiData ;
class ModulationManager;
class MasterEnvelopeSection : public SynthSection {

public:
    MasterEnvelopeSection(juce::ValueTree v, juce::UndoManager &um, OpenGlWrapper &open_gl, SynthGuiData * data, ModulationManager*) : SynthSection("MasterEnvelope") {
        setComponentID("MasterEnvelope");
    }

    void paintBackground(Graphics &g) override;
    void resized() override;
    juce::ValueTree v;
    juce::UndoManager *um;

};



#endif //MASTERENVELOPESECTION_H
