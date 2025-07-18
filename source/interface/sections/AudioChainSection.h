//
// Created by Davis Polito on 6/30/25.
//

#ifndef AUDIOCHAINSECTION_H
#define AUDIOCHAINSECTION_H
#include "synth_section.h"
#include "ModuleList.h"
#include "SoundModuleSection.h"
#include "ParameterView/RoutingView.h"

class ModulesListContainer : public SynthSection {
public:
    ModulesListContainer(String name) : SynthSection(name) {
        setInterceptsMouseClicks(false,true);
        setSidewaysHeading(false);
    }
    void resized() override {
        SynthSection::resized();
    }
    void paintBackground(Graphics& g) override {
        g.fillAll(findColour(Skin::kBackground, true));

        // paintHeadingText(g);
        paintChildrenShadows(g);
        paintChildrenBackgrounds(g);
    }
};
class AudioChainSection : public SynthSection , public ChainList<ProcessorBase>::Listener, ModulesInterface<ProcessorBase>::Listener,
public juce::ScrollBar::Listener, EffectsViewport::Listener {

public:
    AudioChainSection(ChainList<ProcessorBase>&,ModulationManager* m, juce::UndoManager& um);// : SynthSection("AudioChainSection") {}
    ~AudioChainSection();
    void chainAdded(ModuleList<ProcessorBase> *newModule) override;
    void chainChanged() override;
    void removeChain(ModuleList<ProcessorBase> *moduleToRemove) override;
    class Listener {
    public:
        virtual ~Listener() { }
        virtual void effectsMoved() = 0;
        virtual void added() =0;
        virtual void removed() = 0;
    };
    void paintBackground(juce::Graphics& g) override;
    void paintChildrenShadows(juce::Graphics& g) override { }
    void resized() override;
    virtual void redoBackgroundImage();
    // void mouseDown (const juce::MouseEvent& e) override;
    void setFocus() { grabKeyboardFocus(); }
    void setEffectPositions() ;
   void  mouseDown (const juce::MouseEvent& e)
    {
        if(e.mods.isPopupMenu())
        {
            PopupItems options = createPopupMenu();
            showPopupSelector(this, e.getPosition(), options, [=](int selection) { handlePopupResult(selection); });
        }
        juce::Component::mouseDown(e);
    }
    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(juce::OpenGLContext& open_gl) override;

    void scrollBarMoved(ScrollBar* scroll_bar, double range_start) override;
    virtual void setScrollBarRange();
    //    T* createNewObject(const juce::ValueTree& v) override;
    //    void deleteObject (ModuleSection* at) override;
    void reset() override;
    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void effectsScrolled(int position) override {
        setScrollBarRange();
        scroll_bar_->setCurrentRange(position, viewport_.getHeight());
        // DBG("position: " + String(position));
        for (Listener* listener : listeners_)
            listener->effectsMoved();
    }
    void startScroll() override {

    }
    void endScroll() override {

    }

    PopupItems createPopupMenu();
    void handlePopupResult(int result);
    std::map<std::string, SynthSlider *> getAllSliders() override;
    ChainList<ProcessorBase>& chains_;
    std::vector<Listener*> listeners_;
    EffectsViewport viewport_;
    std::unique_ptr<ModulesListContainer> container_;
    OpenGlImage background_;
    CriticalSection open_gl_critical_section_;
    std::unique_ptr<OpenGlScrollBar> scroll_bar_;
    std::vector<std::unique_ptr<SoundModuleSection>> sound_module_sections;
    ModulationManager* modulation_manager_;
    std::shared_ptr<OpenGlQuad> footer_quad_;
    juce::UndoManager& undo;
    void added() override {

        resized();
        // setSize(getWidth(), getHeight()+100);
    }
    void removed() override{
        setSize(getWidth(), getHeight()-100);
    }
    void effectsMoved() override{

    };

};



#endif //AUDIOCHAINSECTION_H
