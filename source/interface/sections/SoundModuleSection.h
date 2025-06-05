//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_SOUNDMODULESECTION_H
#define ELECTROSYNTH_SOUNDMODULESECTION_H
#include "sound_generator_section.h"
class ModuleSection;
class ProcessorBase;
class ModulationManager;
#include "ModuleList.h"
class SoundModuleSection : public ModulesInterface<ProcessorBase>
{
public:
    explicit SoundModuleSection(const juce::ValueTree &, ModulationManager* m, ModuleList<ProcessorBase> &);
    virtual ~SoundModuleSection();

    void setEffectPositions() override;

    PopupItems createPopupMenu() override;
    void handlePopupResult(int result) override;

    std::map<std::string, SynthSlider*> getAllSliders() override;
    std::vector<ModuleSection*> module_sections;
    void moduleAdded(ProcessorBase* newModule) override;


    void removeModule(ProcessorBase* newModule)   override;
void moduleListChanged() ;


};

#endif //ELECTROSYNTH_SOUNDMODULESECTION_H
