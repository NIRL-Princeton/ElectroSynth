//
// Created by Davis Polito on 11/19/24.
//

#ifndef ELECTROSYNTH_ModulationMODULESECTION_H
#define ELECTROSYNTH_ModulationMODULESECTION_H
#include "modules_interface.h"
#include "ModuleList.h"
class ModulatorBase;
class ModulationSection;
class MappingManager;
class ModulationModuleSection : public ModulesInterface<ModulatorBase>
{
public:
    static constexpr int kTabStripHeight = 34;
    static constexpr int kMaxTabs = 8;

    ModulationModuleSection( MappingManager*,ModuleList<ModulatorBase>&, juce::UndoManager& um);
    virtual ~ModulationModuleSection();

     void effectsScrolled(int position) override {
         setScrollBarRange();
         scroll_bar_->setCurrentRange(position, viewport_.getWidth());
        //DBG("pspootion" + juce::String(position));
         for (Listener* listener : listeners_)
             listener->effectsMoved();
     }

    void redoBackgroundImage() override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void paintBackground(juce::Graphics& g) override;
    void setEffectPositions() override;
    void resized() override;
    PopupItems createPopupMenu() override;
    void handlePopupResult(int result) override;

    void mouseEnter(const MouseEvent& event) override;
    void mouseExit(const MouseEvent& event) override;
    std::unique_ptr<OpenGlShapeButton> add_modulator_button_;
    std::shared_ptr<OpenGlQuad> add_mod_button_background_;


    void scrollBarMoved(ScrollBar *scrollBarThatHasMoved, double newRangeStart) override;
    void setScrollBarRange() override;
    void buttonClicked(juce::Button* button) override;
    std::map<std::string, ConnectionButton*> getAllModulationButtons() override;
    void setVCAModulationSection(SynthSection* section, std::shared_ptr<ConnectionButton> mod_button);

     MappingManager* modulation_manager;
     std::shared_ptr<OpenGlQuad> header_body_;
     std::shared_ptr<PlainTextComponent> header_title_;
     std::vector<std::unique_ptr<ModulationSection>> module_sections;
     std::array<std::unique_ptr<OpenGlToggleButton>, kMaxTabs> tab_buttons_;
     std::array<std::shared_ptr<OpenGlQuad>, kMaxTabs> tab_borders_;
     std::array<std::shared_ptr<OpenGlQuad>, kMaxTabs> selected_tab_bottoms_;
     std::array<std::shared_ptr<OpenGlQuad>, kMaxTabs> selected_tab_lefts_;
     std::array<std::shared_ptr<OpenGlQuad>, kMaxTabs> selected_tab_rights_;
     std::array<std::shared_ptr<OpenGlQuad>, kMaxTabs> selected_tab_line_masks_;
 void moduleAdded(ModulatorBase* newModule) override;


 void removeModule(ModulatorBase* newModule)   override;
 void moduleListChanged() ;
 EffectsViewport viewport;
    juce::UndoManager& undo;
private:
    void updateTabs();
    static constexpr int kDefaultTab = -1;
    int getVisibleTabCount() const;
    bool hasVCATab() const { return master_env_section_ != nullptr; }
    int selected_tab_ = kDefaultTab;
    SynthSection* master_env_section_ = nullptr;
    std::shared_ptr<ConnectionButton> master_env_button_;
};

#endif //ELECTROSYNTH_ModulationMODULESECTION_H
