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
#include "Identifiers.h"
#include "ModulationModuleSection.h"
#include "SoundModuleSection.h"
#include "about_section.h"
#include "juce_core/unit_tests/juce_UnitTestCategories.h"
#include "mapping_manager.h"
#include "midi_manager.h"
#include "sound_engine.h"
#include "synth_base.h"
#include "synth_gui_interface.h"
#include "test_section.h"
#include "text_look_and_feel.h"

#include <algorithm>
#include <ranges>

using namespace leaf;

FullInterface::FullInterface(SynthGuiData* synth_data) : SynthSection("full_interface"), width_(0), resized_width_(0),
    last_render_scale_(0.0f), display_scale_(1.0f), pixel_multiple_(1),unsupported_(false), animate_(true), enable_redo_background_(true),
    open_gl_(open_gl_context_), data(synth_data) {

    full_screen_section_ = nullptr;
    Skin default_skin;
    default_skin.copyValuesToLookAndFeel(DefaultLookAndFeel::instance());
    default_skin.copyValuesToLookAndFeel(TextLookAndFeel::instance());
    juce::LookAndFeel::setDefaultLookAndFeel(DefaultLookAndFeel::instance());

    modulation_manager = std::make_unique<MappingManager>(synth_data->tree, synth_data->synth);
    modulation_manager->setOpaque(false);
    modulation_manager->setAlwaysOnTop(true);
    modulation_manager->setConnectionAmounts();
    modulation_manager->setVisibleMeterBounds();
    modulation_manager->hideUnusedHoverModulations();
    modulation_manager->toFront(false);
    addSubSection(modulation_manager.get());

    main_ = std::make_unique<MainSection>(data->tree, data->um, open_gl_, data, modulation_manager.get());
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

    about_section_->toFront(true);
    copySkinValues(default_skin);
    //setOpaque(true);
    open_gl_context_.setContinuousRepainting(true);
    open_gl_context_.setOpenGLVersionRequired(OpenGLContext::openGL3_2);
    open_gl_context_.setSwapInterval(0);
    open_gl_context_.setRenderer(this);
    //componentpaintingenabled fixes flickering
    open_gl_context_.setComponentPaintingEnabled(false);
    open_gl_context_.attachTo(*this);

    ///startTimer(100);
    valueTreeDebugger = new ValueTreeDebugger (synth_data->tree);

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
   current_skin_ = skin;
   skin.copyValuesToLookAndFeel(DefaultLookAndFeel::instance());
   skin.copyValuesToLookAndFeel(TextLookAndFeel::instance());
   setSkinValues(skin, true);
}

void FullInterface::applySkinToSubtree(SynthSection* section) {
   if (section == nullptr)
      return;

   section->setSkinValues(current_skin_, false);
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

    width_ = getWidth();

    if (!enable_redo_background_) {
      // open_gl_context_.detach();
      //juce::DocumentWindow::resized();
      // startTimer(100);
       return;
    }

    resized_width_ = getWidth();

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
    int padding = getPadding() * size_ratio_;
    int voice_padding = findValue(Skin::kLargePadding);

    int main_x = left + 2 * voice_padding;
    int top_height = kTopHeight * ratio;
    int section_one_width = 350 * ratio;
    int section_two_width = section_one_width;
    int audio_width = section_one_width + section_two_width + padding;


    header_->setTabOffset(2 * voice_padding);
    header_->setBounds(left, top, width,  top_height);
    Rectangle<int> main_bounds(left,header_->getBottom(), width, height - top_height );
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

// installs a callback that returns currently selected MIDI output
void FullInterface::setSelectedMidiOutputProvider(std::function<juce::MidiOutput*()> provider) {
    selected_midi_output_provider_ = std::move(provider);
}
// sends all messages in the buffer to the currently selected MIDI output
bool FullInterface::sendMidiBufferToSelectedOutput(const juce::MidiBuffer& midi_messages) {
    if (!selected_midi_output_provider_)
        return false;

    if (auto* selected_output = selected_midi_output_provider_()) {
        selected_output->sendBlockOfMessagesNow(midi_messages);
        return true;
    }

    return false;
}

void FullInterface::sendToDeviceStringChanged(const juce::String& deviceString)
{
    send_to_device_name_ = deviceString;
    DBG("DEVICE STRING: |" + send_to_device_name_ + "|");
}

static void printModuleSmall (const tModuleSmall& module, int i = -1)
{
    if (module.active == 0) return;

    DBG("Module " << ((i < 0) ? "master" : String(i)));
    DBG("  type " << static_cast<int>(module.type));
    DBG("  uuid " << static_cast<int>(module.uuid));
    DBG("  terminal " << static_cast<int>(module.terminal));
    DBG("  chainEnd " << static_cast<int>(module.chainEnd));
    String params = "  params ";
    for (int p = 0; p < MAX_NUM_PARAMS; p++)
    {
        params += " " + String(module.params[p]);
    }
    DBG(params);
}

static void printMappingSmall (const tMappingSmall& mapping, int i = -1)
{
    if (mapping.active == 0) return;

    DBG("Mapping " << ((i < 0) ? "" : String(i)));
    DBG ("  map " << static_cast<int>(mapping.mapUuid));
    DBG("  s_id " << static_cast<int>(mapping.sUuid));
    DBG("  d_id " << static_cast<int>(mapping.dUuid));
    DBG("  d_param " << static_cast<int>(mapping.dParamId));
    DBG("  scalar " << mapping.scalar);
    DBG("  bipolar " << mapping.bipolarOffset);
}

static void printAudioConnectionSmall (const tAudioConnectionSmall& routing, int i = -1)
{
    if (routing.active == 0) return;

    DBG("Routing " << ((i < 0) ? "" : String(i)));
    DBG("  s_id " << static_cast<int>(routing.sUuid));
    DBG("  d_id " << static_cast<int>(routing.dUuid));
    DBG("  amount " << routing.amount);
}

static void printFullPreset(tFullPresetState& preset)
{
    printModuleSmall (preset.master);

    for (int i = 0; i < MAX_NUM_MODULES; i++)
    {
        printModuleSmall (preset.modules[i], i);
    }

    for (int i = 0; i < MAX_NUM_MAPPINGS; i++)
    {
        printMappingSmall (preset.mappings[i], i);
    }

    for (int i = 0; i < MAX_NUM_AUDIO_CONNECTIONS; i++)
    {
        printAudioConnectionSmall (preset.routing[i], i);
    }
}

void FullInterface::sendToDeviceRequested() {
    // value-initialize preset so it doesn't contain indeterminent data

    DBG("SENDING TO DEVICE: " + send_to_device_name_);

    // for Jeff, we get the runtime snapshot here and can use it to create mappingpreset7bits or in another way to get all the data over!
    const auto* synth = data != nullptr ? data->synth : nullptr; // make sure data and data->synth exist
    auto* engine = synth != nullptr ? synth->getEngine() : nullptr; // get engine from synth
    const auto runtimeSnapshot = engine != nullptr ? engine->getRuntimeTopologySnapshot() : nullptr; // get the snapshot !
    if (runtimeSnapshot != nullptr)
    {
        tFullPresetState fullPreset;

        // initialize active flag to 0
        for (int i = 0; i < MAX_NUM_MODULES; i++) fullPreset.modules[i].active = 0;
        for (int i = 0; i < MAX_NUM_MAPPINGS; i++) fullPreset.mappings[i].active = 0;
        for (int i = 0; i < MAX_NUM_AUDIO_CONNECTIONS; i++) fullPreset.routing[i].active = 0;

        std::vector<uint32_t> terminalModules;
        std::vector<uint32_t> chainExitModules;
        for (int i = 0; auto module : runtimeSnapshot->terminalAudioModules) terminalModules.push_back(module.module->getUUID());
        for (int i = 0; auto module : runtimeSnapshot->chainExitModules) chainExitModules.push_back(module->getUUID());

        // Transfer master envelope module, not included in traditional module registry
        const ModuleHeader* masterHeader  = runtimeSnapshot->masterEnvelope->procArray->at(0);
        if (masterHeader != nullptr)
        {
            fullPreset.master.active = 1;
            fullPreset.master.type = masterHeader->moduleType;
            fullPreset.master.uuid = masterHeader->uniqueID;
            fullPreset.master.terminal = 0;
            fullPreset.master.chainEnd = 0;
        }
        for (int p = 0; p < MAX_NUM_PARAMS; p++)
        {
            if (masterHeader->params[p] != nullptr)
                fullPreset.master.params[p] = masterHeader->params[p]->load(std::memory_order_relaxed);
            else
                fullPreset.master.params[p] = 0.0f;
        }

        for (int i = 0; auto module : runtimeSnapshot->modulesToTick)
        {
            if (i >= MAX_NUM_MODULES) break;
            if (module == nullptr || module->procArray == nullptr || module->procArray->empty()) continue;

            const ModuleHeader* header  = module->procArray->at (0);
            if (header == nullptr) continue;

            fullPreset.modules[i].active = 1;
            fullPreset.modules[i].type = header->moduleType;
            fullPreset.modules[i].uuid = header->uniqueID;
            fullPreset.modules[i].terminal = std::ranges::find(terminalModules, header->uniqueID) != terminalModules.end();
            fullPreset.modules[i].chainEnd = std::ranges::find(chainExitModules, header->uniqueID) != chainExitModules.end();

            for (int p = 0; p < MAX_NUM_PARAMS; p++)
            {
                if (header->params[p] != nullptr) 
                    fullPreset.modules[i].params[p] = header->params[p]->load(std::memory_order_relaxed);
                else
                    fullPreset.modules[i].params[p] = 0.0f;
            }
            i++;
        }

        for (int i = 0; auto runtimeMapping : runtimeSnapshot->modulationConnections)
        {
            if (i >= MAX_NUM_MAPPINGS) break;

            const auto mapping = runtimeMapping.state;

            fullPreset.mappings[i].active = 1;
            fullPreset.mappings[i].mapUuid = mapping->mapping.at(0)->uuid;
            fullPreset.mappings[i].sUuid = mapping->source->at(0)->uniqueID;
            fullPreset.mappings[i].dUuid = mapping->destination->at(0)->uniqueID;
            fullPreset.mappings[i].dParamId = mapping->mapping.at(0)->paramID;
            fullPreset.mappings[i].scalar = mapping->record.amount;
            fullPreset.mappings[i].bipolarOffset = mapping->record.bipolar;
            i++;
        }

        for (int i = 0; auto audioConnection : runtimeSnapshot->audioConnections)
        {
            if (i >= MAX_NUM_AUDIO_CONNECTIONS) break;

            fullPreset.routing[i].active = 1;
            fullPreset.routing[i].sUuid = audioConnection.sourceModule->getUUID();
            fullPreset.routing[i].dUuid = audioConnection.destinationModule->getUUID();
            fullPreset.routing[i].amount = audioConnection.amount;
            i++;
        }

        printFullPreset (fullPreset);

        leaf::tFullPresetStateWithNameAndNumber fullPresetWithName {};
        fullPresetWithName.fullstate = fullPreset;
        fullPresetWithName.presetVersionNumberMajor = 1; //this will be some variable stating the version of the preset format, number before the dot
        fullPresetWithName.presetVersionNumberMinor = 0; //this will be some variable stating the version of the preset format, number after the dot
        fullPresetWithName.presetNumber = 8; //this will be a text entry box for which slot to store it in. Putting 8 for now just so I can have a value in there.
        for (int i = 0; i < 14; i++)
        {
            if (i < send_to_device_name_.getNumBytesAsUTF8())
            {
                fullPresetWithName.name[i] = send_to_device_name_[i];
            }
            else
            {
                fullPresetWithName.name[i] = 0;
            }
        }
        //fullPresetWithName.name[0] = 255; //dumb for testing
        // build sysex message
        juce::MidiBuffer midi_messages;
        sendPresetOverMidi(fullPresetWithName, sizeof(fullPresetWithName), midi_messages);

        // send message to currently selected output, or show the alert
        if (!sendMidiBufferToSelectedOutput(midi_messages)) {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "MIDI Output",
                "No valid output selected in Audio/MIDI Settings.");
        }
    }





    //leaf::tMappingPreset7Bit preset7Bit {};


}

void FullInterface::animate(bool animate) {
   if (animate_ != animate)
       open_gl_context_.setContinuousRepainting(animate);

   animate_ = animate;
   SynthSection::animate(animate);
}

void FullInterface::reset() {
    //object creation and destruction is thread safe internally should not need to put a scopedlock hereo
    //vital previously had a scoped lock on this whole section. but this only really makes sense whenever we have a static system
    //since individual components must be capable of being created and destroyed we want to lock or block in those
    //individual destruction calls -- 4/25/25 -- davis

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

void FullInterface::popupTextEntry(juce::Component* source, const std::string& display_text,
    const juce::String& editable_text, juce::BubbleComponent::BubblePlacement placement,
    std::function<void(const juce::String&)> commit, std::function<void()> cancel) {
    PopupDisplay* display = popup_display_1_.get();
    display->setEditableContent(display_text, editable_text,
                                getLocalArea(source, source->getLocalBounds()), placement,
                                std::move(commit), std::move(cancel));
    display->setVisible(true);
    display->toFront(false);
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
       display->dismiss();
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


   juce::ScopedLock lock(open_gl_critical_section_);
   open_gl_.display_scale = display_scale_;
   background_.render (open_gl_);
   modulation_manager->renderMeters(open_gl_, animate_);
   renderOpenGlComponents(open_gl_, animate_);
}

void FullInterface::openGLContextClosing() {
   if (unsupported_)
       return;

   background_.destroy(open_gl_.context);
   destroyOpenGlComponents(open_gl_.context);
   open_gl_.shaders = nullptr;
   shaders_ = nullptr;
}





void FullInterface::modulationChanged() {
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


std::map<std::string, ConnectionButton*> FullInterface::getAllModulationButtons(){
    return main_->getAllModulationButtons();
}




