//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_ModulationMODULESECTION_H
#define ELECTROSYNTH_ModulationMODULESECTION_H
#include "sound_generator_section.h"
class ModulatorBase;
class ModulationSection;
class ModulationManager;
class ModulationModuleSection : public ModulesInterface<ModulationSection>
{
public:
    ModulationModuleSection(juce::ValueTree &, ModulationManager* );
    virtual ~ModulationModuleSection();
    ModulationSection* createNewObject(const juce::ValueTree& v) override;
    void deleteObject (ModulationSection* at) override;

    void scrollBarMoved(ScrollBar* scroll_bar, double range_start) override;
    void setScrollBarRange() override;
    // void reset() override;

    void newObjectAdded (ModulationSection*);

    void objectRemoved (ModulationSection*) override     { resized();}//resized(); }
    void objectOrderChanged() override              {resized(); }//resized(); }
    void valueTreeParentChanged (juce::ValueTree&) override{};
    bool isSuitableType (const juce::ValueTree& v) const override
    {
        return v.hasType (IDs::MODULATOR);
     }
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

     std::map<std::string, ModulationButton*> getAllModulationButtons() override;
     Factory<ModulatorBase> factory;
     ModulationManager* modulation_manager;


};

#endif //ELECTROSYNTH_ModulationMODULESECTION_H
