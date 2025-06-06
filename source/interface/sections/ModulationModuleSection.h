//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_ModulationMODULESECTION_H
#define ELECTROSYNTH_ModulationMODULESECTION_H
#include "sound_generator_section.h"
#include "ModuleList.h"
class ModulatorBase;
class ModulationSection;
class ModulationManager;
class ModulationModuleSection : public ModulesInterface<ModulatorBase>
{
public:
    ModulationModuleSection(const juce::ValueTree &, ModulationManager*,ModuleList<ModulatorBase>& );
    virtual ~ModulationModuleSection();

     void effectsScrolled(int position) override {
         setScrollBarRange();
         scroll_bar_->setCurrentRange(position, viewport_.getWidth());
        DBG("pspootion" + juce::String(position));
         for (Listener* listener : listeners_)
             listener->effectsMoved();
     }
     void redoBackgroundImage() override;
     void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void setEffectPositions() override;
    void resized() override;
     PopupItems createPopupMenu() override;
     void handlePopupResult(int result) override;
     void scrollBarMoved(ScrollBar *scrollBarThatHasMoved, double newRangeStart) override;
    void setScrollBarRange() override;
     std::map<std::string, ModulationButton*> getAllModulationButtons() override;

     ModulationManager* modulation_manager;
     std::vector<ModulationSection*> module_sections;
 void moduleAdded(ModulatorBase* newModule) override;


 void removeModule(ModulatorBase* newModule)   override;
 void moduleListChanged() ;
};

#endif //ELECTROSYNTH_ModulationMODULESECTION_H
