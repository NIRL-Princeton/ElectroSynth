//
// Created by Davis Polito on 11/19/24.
//


#include "ModulationModuleSection.h"
#include "ModulationSection.h"
#include "Modulators/EnvModuleProcessor.h"
#include "Modulators/LFOModuleProcessor.h"
#include "synth_gui_interface.h"
#include "modulation_manager.h"
#include "synth_base.h"

namespace electrosynth {
    class SoundEngine;
}

ModulationModuleSection::ModulationModuleSection(const juce::ValueTree &v, ModulationManager *modulation_manager,ModuleList<ModulatorBase>& module_list) : ModulesInterface(v,module_list), modulation_manager(modulation_manager)
{
    scroll_bar_ = std::make_unique<OpenGlScrollBar>(false);
//    scroll_bar_->setShrinkLeft(true)
    addAndMakeVisible(scroll_bar_.get());
    addOpenGlComponent(scroll_bar_->getGlComponent());
    scroll_bar_->addListener(this);
    //setSkinOverride(Skin::kModulationSection);
    //Skin default_skin;
    //setSkinValues(default_skin,false);
    viewport_.setScrollBarPosition(false,true);
    viewport_.setScrollBarsShown(false, false, false, true);



    addListener(modulation_manager);
    //setInterceptsMouseClicks(false, true);
}

ModulationModuleSection::~ModulationModuleSection()
{

}
void ModulationModuleSection::resized()
{
    static constexpr float kEffectOrderWidthPercent = 0.2f;

    ScopedLock lock(open_gl_critical_section_);

    int order_width = getWidth() * kEffectOrderWidthPercent;
    //    effect_order_->setBounds(0, 0, order_width, getHeight());
    //    effect_order_->setSizeRatio(size_ratio_);
    int large_padding = findValue(Skin::kLargePadding);
    int shadow_width = getComponentShadowWidth();
    int viewport_x = 0 + large_padding - shadow_width;
    int viewport_width = getWidth() - viewport_x - large_padding + 2 * shadow_width;
    viewport_.setBounds(0, 0, getWidth(), getHeight());
    setEffectPositions();

    scroll_bar_->setBounds(0, 0, getWidth(), large_padding - 2);
    scroll_bar_->setColor(findColour(Skin::kLightenScreen, true));

    SynthSection::resized();
}
void ModulationModuleSection::handlePopupResult(int result) {

    //std::vector<vital::ModulationConnection*> connections = getConnections();
    if (result == 1 )
    {
        juce::ValueTree t(IDs::MODULATOR);
        t.setProperty(IDs::type, "env", nullptr);
        list.appendChild(t,nullptr);
    }
    else if (result == 2 )
    {
        juce::ValueTree t(IDs::MODULATOR);
        t.setProperty(IDs::type, "lfo", nullptr);
        list.appendChild(t,nullptr);
    }

}


void ModulationModuleSection::setEffectPositions() {
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    int padding = getPadding();
    int large_padding = findValue(Skin::kLargePadding);
    int shadow_width = getComponentShadowWidth();
    int start_x = 0;
    int effect_width = 200 - start_x - large_padding;
    int knob_section_height = getKnobSectionHeight();
    int widget_margin = findValue(Skin::kWidgetMargin);
    int effect_height = 2 * knob_section_height - widget_margin;
    int y = 0;
    int x = 0;
    juce::Point<int> position = viewport_.getViewPosition();
    //DBG("position viewport: x: " + juce::String(position.getX()) + "y: " + juce::String(position.getY()));
  //  DBG("shadwo width: " + String(shadow_width));
    for(auto& section : module_sections)
    {
        section->setBounds(x, shadow_width, effect_width, effect_height);
        x += effect_width + padding;
    }

    container_->setBounds(0, 0, x - padding + effect_width * 2,viewport_.getHeight());
    viewport_.setViewPosition(position);

    for (Listener* listener : listeners_)
        listener->effectsMoved();

    container_->setScrollWheelEnabled(container_->getWidth() <= viewport_.getWidth());
    setScrollBarRange();
    repaintBackground();
}
PopupItems ModulationModuleSection::createPopupMenu()
{
    PopupItems options;
    options.addItem(1, "add Env" );
    options.addItem(2, "add lfo" );

    return options;
}
void ModulationModuleSection::scrollBarMoved(ScrollBar* scroll_bar, double range_start) {
    viewport_.setViewPosition(juce::Point<int>(range_start,0));
    DBG("rangestart "  + juce::String(range_start));
}

void ModulationModuleSection::setScrollBarRange() {
    scroll_bar_->setRangeLimits(0.0, container_->getWidth() );
    scroll_bar_->setCurrentRange(scroll_bar_->getCurrentRangeStart(), viewport_.getWidth(), dontSendNotification);
    // repaintBackground();
    //DBG("container width " + String(container_->getWidth()));
   // DBG("viewport wdith " + String(viewport_.getWidth()));

   // DBG("scrollbar range: " + String(scroll_bar_->getCurrentRangeStart()) );
}
void ModulationModuleSection::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
    ScopedLock lock(open_gl_critical_section_);
    //    Component* top_level = getTopLevelComponent();
    //    Rectangle<int> global_bounds = top_level->getLocalArea(this, getLocalBounds());
    //    double display_scale = Desktop::getInstance().getDisplays().getDisplayForRect(top_level->getScreenBounds())->scale;
    //    return 1;//
    // display_scale;// * (1.0f * global_bounds.getWidth()) / getWidth();
    OpenGlComponent::setViewPort(&viewport_, open_gl);

    float image_width = background_.getImageWidth(); //electrosynth::utils::nextPowerOfTwo(background_.getImageWidth());
    float image_height =background_.getImageHeight(); // electrosynth::utils::nextPowerOfTwo(background_.getImageHeight());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
    float width_ratio = image_width / (viewport_.getWidth() * mult);
    float height_ratio = image_height / (container_->getHeight() * mult);
    float x_offset = (-2.0f * viewport_.getViewPositionX()) / getWidth();

    background_.setTopLeft(-1.0f  + x_offset, 1.0f);
    background_.setTopRight(-1.0 + 2.0f * width_ratio +  x_offset, 1.0f);
    background_.setBottomLeft(-1.0f  + x_offset, 1.0f - 2.0f * height_ratio);
    background_.setBottomRight(-1.0 + 2.0f * width_ratio + x_offset, 1.0f - 2.0f * height_ratio);

    background_.setColor(Colours::white);
    background_.drawImage(open_gl);

//    DBG("x_offset" + juce::String(x_offset));
    SynthSection::renderOpenGlComponents(open_gl, animate);
}
void ModulationModuleSection::redoBackgroundImage() {
    Colour background = findColour(Skin::kBackground, true);

    int width = std::max(container_->getWidth(), getWidth());
    auto mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
    Image background_image = Image(Image::ARGB, width * mult,  container_->getHeight() * mult, true);

    Graphics background_graphics(background_image);
    background_graphics.addTransform(AffineTransform::scale(mult));
    background_graphics.fillAll(background);
    container_->paintBackground(background_graphics);
    background_.setOwnImage(background_image);
}
std::map<std::string, ModulationButton*> ModulationModuleSection::getAllModulationButtons()
{
    //test_->getAllSliders();
    return container_->getAllModulationButtons();
}
void ModulationModuleSection::moduleAdded(ModulatorBase *newModule) {
    auto *module_section = new ModulationSection( newModule->state, (newModule->createEditor()));
    container_->addSubSection(module_section);
    module_section->setInterceptsMouseClicks(false,true);
    parentHierarchyChanged();
    module_sections.emplace_back(std::move(module_section));
    for(auto listener : listeners_)
    {
        listener->added();
    }
    resized();
}
void ModulationModuleSection::moduleListChanged() {


}

void ModulationModuleSection::removeModule(ModulatorBase *newModule) {
    auto it = std::find_if(module_sections.begin(), module_sections.end(),
        [newModule](ModulationSection* section)
        {
            return section->state == newModule->state;
        });

    if (it != module_sections.end())
    {
        ModulationSection* matchedSection = *it;

        // Do something with matchedSection, e.g. remove from list:
        auto* a = *module_sections.erase(it);
        if ((juce::OpenGLContext::getCurrentContext() == nullptr)) {
            SynthGuiInterface *_parent = findParentComponentOfClass<SynthGuiInterface>();

            //safe to do on message thread because we have locked processing if this is called
            a->setVisible(false);
            _parent->getOpenGlWrapper()->context.executeOnGLThread([this, a](juce::OpenGLContext &openGLContext) {
                                                    a->destroyOpenGlComponents(openGLContext);
                                                  this->container_->removeSubSection(a);
                                              },
                                              false);
        } else
            delete a;

    }
    resized();
}

