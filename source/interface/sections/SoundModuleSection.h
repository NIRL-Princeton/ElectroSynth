//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_SOUNDMODULESECTION_H
#define ELECTROSYNTH_SOUNDMODULESECTION_H
#include "sound_generator_section.h"
class ModuleSection;
class ProcessorBase;
class ModulationManager;
class SoundModuleSection : public ModulesInterface<ModuleSection>
{
public:
    explicit SoundModuleSection(juce::ValueTree &, ModulationManager* m);
    virtual ~SoundModuleSection();
    ModuleSection* createNewObject(const juce::ValueTree& v) override;
    void deleteObject (ModuleSection* at) override;


    // void reset() override;
//    void newObjectAdded (ModuleSection*) override;
    void objectRemoved (ModuleSection*) override     { resized();}//resized(); }
    void objectOrderChanged() override              {resized(); }//resized(); }
    void valueTreeParentChanged (juce::ValueTree&) override{};
//    void valueTreeRedirected (juce::ValueTree&) override;
    bool isSuitableType (const juce::ValueTree& v) const override
    {
        return v.hasType (IDs::MODULE);
    }
    void setEffectPositions() override;

    PopupItems createPopupMenu() override;
    void handlePopupResult(int result) override;
    Factory<ProcessorBase> factory;
    std::map<std::string, SynthSlider*> getAllSliders() override;

};

#endif //ELECTROSYNTH_SOUNDMODULESECTION_H
