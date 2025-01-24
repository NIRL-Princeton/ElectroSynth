/* Copyright 2013-2019 Matt Tytel
*
* vital is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* vital is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with vital.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FullInterface.h"
#include "text_look_and_feel.h"
#include "Identifiers.h"
#include "about_section.h"
#include "synth_gui_interface.h"
#include "SoundModuleSection.h"
#include "ModulationModuleSection.h"
#include "modulation_manager.h"
#include "test_section.h"

FullInterface::FullInterface(SynthGuiData* synth_data) : SynthSection("full_interface"), width_(0), resized_width_(0),
                                                         last_render_scale_(0.0f), display_scale_(1.0f),
                                                         pixel_multiple_(1),unsupported_(false), animate_(true),
                                                        enable_redo_background_(true),
                                                         open_gl_(open_gl_context_),
                                                          data(synth_data)
{
    full_screen_section_ = nullptr;
    Skin default_skin;
    setSkinValues(default_skin, true);
    default_skin.copyValuesToLookAndFeel(DefaultLookAndFeel::instance());



    ValueTree t(IDs::PIANO);
    t.setProperty(IDs::name, "default", nullptr);


    data->tree.addChild(t, -1, nullptr);

    modulation_manager = std::make_unique<ModulationManager>(t, synth_data->synth);
    modulation_manager->setOpaque(false);
    modulation_manager->setAlwaysOnTop(true);
    modulation_manager->setModulationAmounts();
    modulation_manager->setVisibleMeterBounds();
    modulation_manager->hideUnusedHoverModulations();
    modulation_manager->toFront(false);


    main_ = std::make_unique<MainSection>(data->tree.getChildWithName(IDs::PIANO), data->um, open_gl_, data, modulation_manager.get());
    addSubSection(main_.get());
    main_->addListener(this);


    header_ = std::make_unique<HeaderSection>();
    addSubSection(header_.get());
    header_->addListener(this);
    //inspectButton = std::make_unique<OpenGlToggleButton>("Inspect the UI");
//    addAndMakeVisible(inspectButton.get());
//    addOpenGlComponent(inspectButton->getGlComponent());
//    inspectButton->setText("Inspect");
//    // this chunk of code instantiates and opens the melatonin inspector
//    inspectButton->onClick = [&] {
//        if (!inspector)
//        {
//            inspector = std::make_unique<melatonin::Inspector> (*this);
//            inspector->onClose = [this]() { inspector.reset(); };
//        }
//
//        inspector->setVisible (true);
//    };
//    inspectButton->setAlwaysOnTop(true);
    popup_selector_ = std::make_unique<SinglePopupSelector>();
    addSubSection(popup_selector_.get());
    popup_selector_->setVisible(false);
    popup_selector_->setAlwaysOnTop(true);
    popup_selector_->setWantsKeyboardFocus(true);




    popup_display_1_ = std::make_unique<PopupDisplay>();
    addSubSection(popup_display_1_.get());
    popup_display_1_->setVisible(false);
    popup_display_1_->setAlwaysOnTop(true);
    popup_display_1_->setWantsKeyboardFocus(false);

    popup_display_2_ = std::make_unique<PopupDisplay>();
    addSubSection(popup_display_2_.get());
    popup_display_2_->setVisible(false);
    popup_display_2_->setAlwaysOnTop(true);
    popup_display_2_->setWantsKeyboardFocus(false);

    about_section_ = std::make_unique<AboutSection>("about");
    addSubSection(about_section_.get(), false);
    addChildComponent(about_section_.get());

    addSubSection(modulation_manager.get());

    about_section_->toFront(true);
    //setOpaque(true);
    open_gl_context_.setContinuousRepainting(true);
    open_gl_context_.setOpenGLVersionRequired(OpenGLContext::openGL3_2);
    open_gl_context_.setSwapInterval(0);
    open_gl_context_.setRenderer(this);
    //componentpaintingenabled fixes flickering
    open_gl_context_.setComponentPaintingEnabled(false);
    open_gl_context_.attachTo(*this);

    ///startTimer(100);

}

//FullInterface::FullInterface() : SynthSection("EMPTY"), open_gl_(open_gl_context_) {
//  // Skin default_skin;
//   //setSkinValues(default_skin, true);
//
//   open_gl_context_.setContinuousRepainting(true);
//   open_gl_context_.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
//   open_gl_context_.setSwapInterval(0);
//   open_gl_context_.setRenderer(this);
//   open_gl_context_.setComponentPaintingEnabled(false);
//   open_gl_context_.attachTo(*this);
//
//   reset();
//   setOpaque(true);
//}

FullInterface::~FullInterface() {
   //UpdateMemory::getInstance()->decrementChecker();

   open_gl_context_.detach();
   open_gl_context_.setRenderer(nullptr);
}

void FullInterface::paintBackground(juce::Graphics& g) {
   g.fillAll(findColour(Skin::kBackground, true));
   paintChildrenShadows(g);

   int padding = getPadding();
   int bar_width = 6 * padding;
   //g.setColour(header_->findColour(Skin::kBody, true));
   //g.setColour(main_->findColour(Skin::kBody, true));
   //g.setColour(inspectButton->findColour(Skin::k)
   //int y = header_->getBottom();
//   int height = keyboard_interface_->getY() - y;
//   int x1 = extra_mod_section_->getRight() + padding;
//   g.fillRect(x1, y, bar_width, height);

//   if (synthesis_interface_) {
//       int x2 = synthesis_interface_->getRight() + padding;
//       g.fillRect(x2, y, bar_width, height);
//   }

   paintKnobShadows(g);
   paintChildrenBackgrounds(g);
}

void FullInterface::copySkinValues(const Skin& skin) {
//   ScopedLock open_gl_lock(open_gl_critical_section_);
//   skin.copyValuesToLookAndFeel(DefaultLookAndFeel::instance());
//   setSkinValues(skin, true);
}

void FullInterface::reloadSkin(const Skin& skin) {
   copySkinValues(skin);
   juce::Rectangle<int> bounds = getBounds();
   setBounds(0, 0, bounds.getWidth() / 4, bounds.getHeight() / 4);
   setBounds(bounds);
}


void FullInterface::repaintChildBackground(SynthSection* child) {
   if (!background_image_.isValid())
       return;
//
//   if (child->getParentComponent() == synthesis_interface_.get()) {
//       repaintSynthesisSection();
//       return;
//   }
//
//   if (effects_interface_ != nullptr && effects_interface_->isParentOf(child))
//       child = effects_interface_.get();

   background_.lock();
   juce::Graphics g(background_image_);
   paintChildBackground(g, child);
   background_.updateBackgroundImage(background_image_);
   background_.unlock();
}

void FullInterface::repaintSynthesisSection() {
   if (!background_image_.isValid())
      return;

   background_.lock();
   juce::Graphics g(background_image_);
   int padding = findValue(Skin::kPadding);
   g.setColour(findColour(Skin::kBackground, true));
//   g.fillRect(synthesis_interface_->getBounds().expanded(padding));
//   paintChildShadow(g, synthesis_interface_.get());
//   paintChildBackground(g, synthesis_interface_.get());

   background_.updateBackgroundImage(background_image_);
   background_.unlock();
}

void FullInterface::repaintOpenGlBackground(OpenGlComponent* component) {
   if (!background_image_.isValid())
       return;

   background_.lock();
   juce::Graphics g(background_image_);
   paintOpenGlBackground(g, component);
   background_.updateBackgroundImage(background_image_);
   background_.unlock();
}

void FullInterface::redoBackground() {
   int width = std::ceil(display_scale_ * getWidth());
   int height = std::ceil(display_scale_ * getHeight());
   if (width < electrosynth::kMinWindowWidth || height < electrosynth::kMinWindowHeight)
       return;

   juce::ScopedLock open_gl_lock(open_gl_critical_section_);

   background_.lock();
   background_image_ = juce::Image(juce::Image::RGB, width, height, true);
   juce::Graphics g(background_image_);
   paintBackground(g);
   background_.updateBackgroundImage(background_image_);
   background_.unlock();
}

void FullInterface::checkShouldReposition(bool resize) {
   float old_scale = display_scale_;
   int old_pixel_multiple = pixel_multiple_;
   display_scale_ = getDisplayScale();
   pixel_multiple_ = std::max<int>(1, display_scale_);

   if (resize && (old_scale != display_scale_ || old_pixel_multiple != pixel_multiple_))
       resized();

}

void FullInterface::resized() {
    checkShouldReposition(false);
//SynthSection::resized();

   if (!enable_redo_background_)
   {
      // open_gl_context_.detach();
      //juce::DocumentWindow::resized();
      // startTimer(100);
       return;
   }
   width_ = getWidth();


   resized_width_ = width_;

   juce::ScopedLock lock(open_gl_critical_section_);
   static constexpr int kTopHeight = 48;

//   if (effects_interface_ == nullptr)
//       return;

   int left = 0;
   int top = 0;
   int width = std::ceil(getWidth() * display_scale_);
   int height = std::ceil(getHeight() * display_scale_);
   juce::Rectangle<int> bounds(0, 0, width, height);
   modulation_manager->setBounds(bounds);
   float width_ratio = getWidth() / (1.0f * electrosynth::kDefaultWindowWidth);
   float ratio = width_ratio * display_scale_;
   float height_ratio = getHeight() / (1.0f * electrosynth::kDefaultWindowHeight);
   if (width_ratio > height_ratio + 1.0f / electrosynth::kDefaultWindowHeight) {
       ratio = height_ratio;
       width = height_ratio * electrosynth::kDefaultWindowWidth * display_scale_;
       left = (getWidth() - width) / 2;
   }
   if (height_ratio > width_ratio + 1.0f / electrosynth::kDefaultWindowHeight) {
       ratio = width_ratio;
       height = ratio * electrosynth::kDefaultWindowHeight * display_scale_;
       top = (getHeight() - height) / 2;
   }

   setSizeRatio(ratio);
//   DBG("");
//    DBG( "ratio: " + juce::String(ratio));
//    DBG("display scale" + juce::String(display_scale_));
   int padding = getPadding();
   int voice_padding = findValue(Skin::kLargePadding);

   int main_x = left + 2 * voice_padding;
   int top_height = kTopHeight * ratio;
   int section_one_width = 350 * ratio;
   int section_two_width = section_one_width;
   int audio_width = section_one_width + section_two_width + padding;


   header_->setTabOffset(2 * voice_padding);
   header_->setBounds(left, top, width,  top_height);
   Rectangle<int> main_bounds(left,header_->getBottom(), width, height - top_height);
   juce::Rectangle<int> new_bounds(0, 0, width, height);
   main_->setBounds(main_bounds);
   //test_section->setBounds(main_bounds);
  // prep_popup->setBounds(100, 100, 700, 700);
   about_section_->setBounds(new_bounds);
   //inspectButton->setBounds(10, 0, 100, 100);
   if (getWidth() && getHeight())
       redoBackground();

   //open_gl_context_.attachTo(*this);
}


void FullInterface::showAboutSection()
{
    juce::ScopedLock lock(open_gl_critical_section_);
    about_section_->setVisible(true);
}
void FullInterface::animate(bool animate) {
   if (animate_ != animate)
       open_gl_context_.setContinuousRepainting(animate);

   animate_ = animate;
   SynthSection::animate(animate);
}

void FullInterface::reset() {
   juce::ScopedLock lock(open_gl_critical_section_);
//   if(main_ != nullptr && !main_->v.getParent().isValid()){
//       main_->v.copyPropertiesAndChildrenFrom(data->mainSynth->getValueTree().getChild(0), nullptr);
//   }

   SynthSection::reset();
   DBG("critical opengl");
   repaintSynthesisSection();
    DBG("critical opengl over");
}

void FullInterface::popupDisplay(juce::Component* source, const std::string& text,
    juce::BubbleComponent::BubblePlacement placement, bool primary) {
   PopupDisplay* display = primary ? popup_display_1_.get() : popup_display_2_.get();
   display->setContent(text, getLocalArea(source, source->getLocalBounds()), placement);
   display->setVisible(true);
}

//void FullInterface::prepDisplay(PreparationSection* prep)
//{
//    DBG("*********SETTING CONTENT***************");
//    prep_popup->setContent(prep->getPrepPopup());
//    DBG("*********CONTEN SET***************");
//    prep_popup->setVisible(true);
//}
//
void FullInterface::hideDisplay(bool primary) {
   PopupDisplay* display = primary ? popup_display_1_.get() : popup_display_2_.get();
   if (display)
       display->setVisible(false);
}

void FullInterface::popupSelector(juce::Component* source, juce::Point<int> position, const PopupItems& options,
    std::function<void(int)> callback, std::function<void()> cancel) {
   popup_selector_->setCallback(callback);
   popup_selector_->setCancelCallback(cancel);
   popup_selector_->showSelections(options);
   juce::Rectangle<int> bounds(0, 0, std::ceil(display_scale_ * getWidth()), std::ceil(display_scale_ * getHeight()));
   popup_selector_->setPosition(getLocalPoint(source, position), bounds);
   popup_selector_->setVisible(true);
}



void FullInterface::newOpenGLContextCreated() {
   double version_supported = juce::OpenGLShaderProgram::getLanguageVersion();
   unsupported_ = version_supported < kMinOpenGlVersion;
   if (unsupported_) {
       juce::NativeMessageBox::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Unsupported OpenGl Version",
           juce::String("Vial requires OpenGL version: ") + juce::String(kMinOpenGlVersion) +
               juce::String("\nSupported version: ") + juce::String(version_supported));
       return;
   }

   shaders_ = std::make_unique<Shaders>(open_gl_context_);
   open_gl_.shaders = shaders_.get();
   open_gl_.display_scale = display_scale_;
   last_render_scale_ = display_scale_;

   background_.init(open_gl_);
   initOpenGlComponents(open_gl_);
   //main_.setContext(open_gl_);
}

void FullInterface::renderOpenGL() {
   if (unsupported_)
       return;

   float render_scale = open_gl_.context.getRenderingScale();
   //DBG(render_scale);
   if (render_scale != last_render_scale_) {
       last_render_scale_ = render_scale;
       juce::MessageManager::callAsync([=] { checkShouldReposition(true); });
   }
//   if(!open_gl_.init_comp.empty())
//    {
//       OpenGlComponent* comp = dynamic_cast<OpenGlComponent*>(open_gl_.init_comp.back());
//       comp->init(open_gl_);
//       open_gl_.init_comp.pop_back();
//
//    }


/// initialize opengl components dynamically
   OpenGlWrapper::glInitAction action;
   while (open_gl_.initOpenGlComp.try_dequeue(action))
       action();


   juce::ScopedLock lock(open_gl_critical_section_);
   open_gl_.display_scale = display_scale_;
   background_.render (open_gl_);
   modulation_manager->renderMeters(open_gl_, animate_);
   renderOpenGlComponents(open_gl_, animate_);
}

void FullInterface::openGLContextClosing() {
   if (unsupported_)
       return;

   background_.destroy(open_gl_);
   destroyOpenGlComponents(open_gl_);
   open_gl_.shaders = nullptr;
   shaders_ = nullptr;
}





void FullInterface::modulationChanged()
{
    if (modulation_manager)
        modulation_manager->reset();
}



void FullInterface::setFocus() {

}

void FullInterface::notifyChange() {
//   if (header_)
//       header_->notifyChange();
}

void FullInterface::notifyFresh() {
//   if (header_)
//       header_->notifyFresh();
}



void FullInterface::showFullScreenSection(SynthSection* full_screen) {
   juce::ScopedLock lock(open_gl_critical_section_);
   full_screen_section_ = full_screen;

   if (full_screen_section_) {
       addSubSection(full_screen_section_);
       full_screen_section_->setBounds(getLocalBounds());
   }

//   for (int i = 0; i < electrosynth::kNumOscillators; ++i)
//       wavetable_edits_[i]->setVisible(false);
//

 bool show_rest = full_screen == nullptr;
  // header_->setVisible(show_rest);
//   synthesis_interface_->setVisible(show_rest);
//   modulation_interface_->setVisible(show_rest);
//   keyboard_interface_->setVisible(show_rest);
//   extra_mod_section_->setVisible(show_rest);
//   modulation_manager_->setVisible(show_rest);
//   voice_section_->setVisible(show_rest);
//   bend_section_->setVisible(show_rest);
//   portamento_section_->setVisible(show_rest);
   redoBackground();
}

//void FullInterface::showWavetableEditSection(int index) {
//   if (!wavetableEditorsInitialized())
//       return;
//
//   ScopedLock lock(open_gl_critical_section_);
//   for (int i = 0; i < electrosynth::kNumOscillators; ++i)
//       wavetable_edits_[i]->setVisible(i == index);
//
//   bool show_rest = index < 0;
//   header_->setVisible(show_rest);
//   synthesis_interface_->setVisible(show_rest);
//   modulation_interface_->setVisible(show_rest);
//   keyboard_interface_->setVisible(show_rest);
//   extra_mod_section_->setVisible(show_rest);
//   modulation_manager_->setVisible(show_rest);
//   voice_section_->setVisible(show_rest);
//   bend_section_->setVisible(show_rest);
//   portamento_section_->setVisible(show_rest);
//   redoBackground();
//}



std::map<std::string, SynthSlider*> FullInterface::getAllSliders(){
    return main_->getAllSliders();
}


std::map<std::string, ModulationButton*> FullInterface::getAllModulationButtons(){
    return main_->getAllModulationButtons();
}









