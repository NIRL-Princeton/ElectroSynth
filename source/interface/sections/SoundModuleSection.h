//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_SOUNDMODULESECTION_H
#define ELECTROSYNTH_SOUNDMODULESECTION_H
#include "modules_interface.h"
class ModuleSection;
class ProcessorBase;
class ModulationManager;
#include "ModuleList.h"
class SoundModuleSection : public ModulesInterface<ProcessorBase>
{
public:
    explicit SoundModuleSection( ModulationManager* m, ModuleList<ProcessorBase> &);
    virtual ~SoundModuleSection();

    void setEffectPositions() override;

    PopupItems createPopupMenu() override;
    void handlePopupResult(int result) override;

    std::map<std::string, SynthSlider*> getAllSliders() override;
    std::vector<std::unique_ptr<ModuleSection>> module_sections;
    void moduleAdded(ProcessorBase* newModule) override;


    void removeModule(ProcessorBase* newModule)   override;
void moduleListChanged() ;


};

#endif //ELECTROSYNTH_SOUNDMODULESECTION_H
