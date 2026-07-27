//
// Created by Davis Polito on 11/19/24.
//

#include "EffectsModuleSection.h"

#include "EffectList.h"
#include "ModuleSection.h"
#include "Processors/ProcessorBase.h"
#include "about_section.h"
#include "modulation_manager.h"
#include "synth_base.h"
#include "synth_gui_interface.h"

namespace {
// Drag-mode boundary bands between adjacent non-dragged modules: pool size and
// band height (centered on the shared module edge, in logical units).
constexpr int kMaxDragBoundaryBands = 16;
constexpr int kDragBoundaryBandHeight = 8;

class AddButtonBackground final : public OpenGlImageComponent {
public:
    AddButtonBackground() : OpenGlImageComponent("effect_add_button_background") {
        setInterceptsMouseClicks(false, false);
    }

    void resized() override {
        redrawImage(true);
    }

    void paintToImage(juce::Graphics& g) override {

        const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        const auto base = findColour(Skin::kBorder, true);
        g.setColour(findColour(Skin::kBackground, true).withAlpha(0.45f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 1.0f), 5.0f);

        juce::ColourGradient gradient(base.brighter(0.08f), 0.0f, bounds.getY(),
                                      base.darker(0.10f), 0.0f, bounds.getBottom(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, 5.0f);

    }
};
}

EffectModuleSection::EffectModuleSection(ModulationManager *m, AudioRoutingManager* arm, EffectList &module_list,const juce::ValueTree &v, juce::UndoManager& um) :
audio_routing_manager_(arm), ModulesInterface( module_list), footer_body(new OpenGlQuad(Shaders::kRoundedRectangleFragment)), state(v), undo(um)
{
    scroll_bar_ = std::make_unique<OpenGlScrollBar>();
    addAndMakeVisible(scroll_bar_.get());
    addOpenGlComponent(scroll_bar_->getGlComponent());

    container_->addOpenGlComponent(footer_body);

    setLookAndFeel(DefaultLookAndFeel::instance());
    scroll_bar_->addListener(this);
    viewport_.setScrollBarPosition(true, false); //use this to determine viewport scroll type in effectsviewport
    viewport_.setScrollBarsShown(false, false, true, false);

    addListener(m);
    for (auto obj : list) {
        EffectModuleSection::moduleAdded(obj);
    }
    setSidewaysHeading(false);
    // Lane label derives from the EffectList's lane index (0/1/2) rather than a
    // hardcoded "FX" string, so the three lanes read "Lane 1/2/3".
    setName("Lane " + juce::String(module_list.lane + 1));

    header_body_ = std::make_shared<OpenGlQuad>(Shaders::kColorFragment, "effect_module_header");
    header_body_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(header_body_, true);

    header_title_ = std::make_shared<PlainTextComponent>("effect_module_title", getName());
    header_title_->setFontType(PlainTextComponent::kLight);
    header_title_->setJustification(juce::Justification::centred);
    header_title_->setInterceptsMouseClicks(false, false);
    addOpenGlComponent(header_title_);

    // FX routing state is not currently exposed. Keep the lane-header control
    // visibly present but disabled rather than attaching it to sound-chain routing.
    routing_combo_box_ = std::make_unique<OpenGLComboBox>();
    routing_combo_box_->addItem("Master", 1);
    routing_combo_box_->setSelectedId(1, juce::dontSendNotification);
    routing_combo_box_->setInterceptsMouseClicks(false, false);
    routing_combo_box_->setWantsKeyboardFocus(false);
    addAndMakeVisible(routing_combo_box_.get());
    addOpenGlComponent(routing_combo_box_->getImageComponent());

    add_effect_button_background_ = std::make_shared<AddButtonBackground>();
    container_->addOpenGlComponent(add_effect_button_background_);

    add_effect_button_ = std::make_unique<OpenGlShapeButton>("Add Effect");
    container_->addAndMakeVisible(add_effect_button_.get());
    container_->addOpenGlComponent(add_effect_button_->getGlComponent());
    add_effect_button_->addListener(this);
    add_effect_button_->addMouseListener(this, false);
    add_effect_button_->setShape(Paths::plus(150));

    // Two coincident overlay passes mirror ModulationModuleSection::paintBackground(),
    // which paints the same 1.0 outline twice. They remain above the scrolling content.
    border_overlay_ = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleBorderFragment, "effect_lane_border");
    border_overlay_second_pass_ = std::make_shared<OpenGlQuad>(
        Shaders::kRoundedRectangleBorderFragment, "effect_lane_border_second_pass");
    border_overlay_->setInterceptsMouseClicks(false, false);
    border_overlay_second_pass_->setInterceptsMouseClicks(false, false);
    border_overlay_->setAlwaysOnTop(true);
    border_overlay_second_pass_->setAlwaysOnTop(true);
    addOpenGlComponent(border_overlay_);
    addOpenGlComponent(border_overlay_second_pass_);

    // Drop-location preview: a translucent FX-accent region filling the gap where the
    // dragged module will land. Owned by the container so it renders above the baked
    // scroll image and non-dragged modules, but below the always-on-top dragged module.
    insertion_region_ = std::make_shared<OpenGlQuad>(Shaders::kRoundedRectangleFragment,
                                                     "effect_insertion_region");
    insertion_region_->setInterceptsMouseClicks(false, false);
    insertion_region_->setAlpha(0.0f, true);
    insertion_region_->setScissorComponent(&viewport_);
    container_->addOpenGlComponent(insertion_region_);

    // Drag-mode boundary bands: one thin translucent FX-accent band per boundary
    // between adjacent non-dragged modules. Same container GL layer as the
    // insertion region, so they render above the dimmed modules' overlays but
    // below the always-on-top dragged module.
    drag_boundary_bands_ = std::make_shared<OpenGlMultiQuad>(kMaxDragBoundaryBands,
                                                             Shaders::kColorFragment,
                                                             "effect_drag_boundary_bands");
    drag_boundary_bands_->setInterceptsMouseClicks(false, false);
    drag_boundary_bands_->setAlpha(0.0f, true);
    drag_boundary_bands_->setNumQuads(0);
    drag_boundary_bands_->setScissorComponent(&viewport_);
    container_->addOpenGlComponent(drag_boundary_bands_);

    toggle_button_->setVisible(false);
    setInterceptsMouseClicks(true,true);

    // initialize routing UI
    if (module_list.getAudioNodeDescriptor().hasOutput) { // if this module supports outputs...
        electrosynth::audio::AudioPortAddress address { // give it an output audio port address
            module_list.getAudioNodeId(),
            module_list.getAudioNodeDescriptor().outputPortId,
            electrosynth::audio::PortDirection::Output,
            module_list.getAudioNodeDescriptor().domain
        };
        lane_output_port_ = std::make_shared<AudioPortComponent>( // make an output arrow belonging to this output port
            "audio_output",
            std::move(address));
        addOpenGlComponent(lane_output_port_);

        lane_output_slots_ = std::make_unique<AudioConnectionSlots>(*lane_output_port_);
        addSubSection(lane_output_slots_.get());
        lane_output_slots_->setConnections({});
    }

    if (module_list.getAudioNodeDescriptor().hasInput) { // if this module supports inputs...
        electrosynth::audio::AudioPortAddress address { // give it an output audio port address
            module_list.getAudioNodeId(),
            module_list.getAudioNodeDescriptor().inputPortId,
            electrosynth::audio::PortDirection::Input,
            module_list.getAudioNodeDescriptor().domain
        };
        lane_input_port_ = std::make_shared<AudioPortComponent>( // make an output arrow belonging to this output port
            "audio_input",
            std::move(address));
        addOpenGlComponent(lane_input_port_);

        lane_input_slots_ = std::make_unique<AudioConnectionSlots>(*lane_input_port_);
        addSubSection(lane_input_slots_.get());
        lane_input_slots_->setConnections({});
    }

    if (audio_routing_manager_ != nullptr) {
        if (lane_output_port_ != nullptr)
            audio_routing_manager_->registerPort(*lane_output_port_);

        if (lane_input_port_ != nullptr)
            audio_routing_manager_->registerPort(*lane_input_port_);
    }

    setSkinOverride(Skin::kFx);
}

EffectModuleSection::~EffectModuleSection() {
    if (audio_routing_manager_ != nullptr) {
        if (lane_input_port_)
            audio_routing_manager_->unregisterPort(*lane_input_port_);

        if (lane_output_port_)
            audio_routing_manager_->unregisterPort(*lane_output_port_);
    }

   module_sections.clear();
}

void EffectModuleSection::handlePopupResult(int result) {
    //std::vector<vital::ModulationConnection*> connections = getConnections();
    // if (result == 1) {
    //     juce::ValueTree t(IDs::EffectMODULE);
    //     t.setProperty(IDs::type, "osc", nullptr);
    //     list.appendChild(t, nullptr);
    if (result == 1) {
        juce::ValueTree t(IDs::EFFECTMODULE);
        t.setProperty(IDs::type, "filt", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    }
    else if (result == 2) {
        juce::ValueTree t(IDs::EFFECTMODULE);
        t.setProperty(IDs::type, "delay", nullptr);
        undo.beginNewTransaction();
        list.appendChild(t, &undo);
    }
}


void EffectModuleSection::setEffectPositions() {
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    // Capture the scroll position before any layout: moving/resizing the container
    // makes the viewport recompute its stored position, so reading it afterwards
    // (the old "preserve scroll" pattern) returned an already-clobbered value.
    const auto view_position = viewport_.getViewPosition();

    // No vertical gap between stacked FX modules (FX-local; SoundModuleSection unaffected).
    int padding = findValue(Skin::kPadding);
    int initial_x = padding;
    int initial_y = 0;
    int effect_width = getWidth() - 2 * padding;
    juce::Rectangle<int> gap_bounds;
    // Boundaries between two adjacent non-dragged modules; edges touching the
    // insertion gap are excluded (the gap region itself marks those).
    std::vector<int> boundary_ys;
    bool previous_was_module = false;

    int delay_index = 1;
    int filt_index = 1;
    for (int i = 0; i < module_sections.size(); ++i) {
        ModuleSection* section = module_sections[i].get();
        const auto type = section->state.getProperty(IDs::type).toString();
        if (type == "delay")
            section->setName("Delay " + juce::String(delay_index++));
        else if (type == "filt")
            section->setName("Filter " + juce::String(filt_index++));


        // During a drag the dragged module floats at its pointer-driven position;
        // its slot in the stack stays open as the insertion gap.
        if (section == dragged_module_) {
            gap_bounds = { initial_x, initial_y, effect_width, section->height };
            initial_y += section->height + padding;
            previous_was_module = false;
            continue;
        }
        if (previous_was_module)
            boundary_ys.push_back(initial_y);

        previous_was_module = true;

        // Size each FX module from its contained view's dynamic row layout instead of the
        // full viewport height. Set bounds once first so the FX view gets its width and can
        // compute a width-dependent row count, then read the preferred height and apply it.
        // (FX-local: SoundModuleSection sizes its modules separately.)
        section->setBounds(initial_x, initial_y, effect_width, section->height);
        section->height = section->getPreferredHeight();
        section->setDrawBottomSeparator(i + 1 < module_sections.size());
        section->setBounds(initial_x, initial_y, effect_width, section->height);
        initial_y += section->height + padding;
    }

    if (insertion_region_ != nullptr) {
        if (dragged_module_ != nullptr && !gap_bounds.isEmpty())
            insertion_region_->setBounds(gap_bounds);  // container coordinates
        else
            insertion_region_->setAlpha(0.0f);
    }

    if (drag_boundary_bands_ != nullptr) {
        const int num_bands = std::min((int)boundary_ys.size(), kMaxDragBoundaryBands);
        if (dragged_module_ != nullptr && num_bands > 0 && initial_y > 0) {
            // The multi-quad spans the whole container; each band is placed in the
            // quad's normalized GL space (y up, so the component top maps to +1).
            drag_boundary_bands_->setBounds(0, 0, viewport_.getWidth(), initial_y);
            drag_boundary_bands_->setNumQuads(num_bands);
            const float total_height = (float)initial_y;
            const float band_height_gl = 2.0f * kDragBoundaryBandHeight / total_height;
            for (int i = 0; i < num_bands; ++i) {
                const float band_bottom = boundary_ys[i] + kDragBoundaryBandHeight / 2.0f;
                drag_boundary_bands_->setQuad(i, -1.0f, 1.0f - 2.0f * band_bottom / total_height,
                                              2.0f, band_height_gl);
            }
        }
        else
            drag_boundary_bands_->setNumQuads(0);
    }

    const int button_width = static_cast<int>(findValue(Skin::kAddButtonSize));
    const int button_x = viewport_.getWidth() / 2 - button_width / 2;
    const int button_y = initial_y + 10;
    const int footer_padding = 35;
    const int container_height = button_y + button_width + footer_padding;

    container_->setSize(viewport_.getWidth(), container_height);

    footer_body->setBounds(0, button_y, viewport_.getWidth(), findValue(Skin::kLargePadding));
    add_effect_button_->setBounds(button_x, button_y, button_width, button_width);
    add_effect_button_background_->setBounds(add_effect_button_->getBounds().reduced(4));
    viewport_.setViewPosition(view_position);
}


PopupItems EffectModuleSection::createPopupMenu() {
    PopupItems options;
    options.addItem(1, "add filt");
    options.addItem(2, "add delay");
    return options;
}

void EffectModuleSection::buttonClicked(juce::Button* button) {
    if (button == add_effect_button_.get()) {
        hidePopupDisplay(true);
        showPopupSelector(add_effect_button_.get(), add_effect_button_->getLocalBounds().getCentre(),
                          createPopupMenu(), [this](int selection) { handlePopupResult(selection); });
        return;
    }

    ModulesInterface<ProcessorBase>::buttonClicked(button);
}

void EffectModuleSection::mouseEnter(const juce::MouseEvent& event) {
    if (event.eventComponent == add_effect_button_.get()) {
        showPopupDisplay(add_effect_button_.get(), "Click to add effect",
                         juce::BubbleComponent::left, true);
    }
}

void EffectModuleSection::mouseExit(const juce::MouseEvent& event) {
    if (event.eventComponent == add_effect_button_.get())
        hidePopupDisplay(true);
}


std::map<std::string, SynthSlider *> EffectModuleSection::getAllSliders() {
    return container_->getAllSliders();
}

void EffectModuleSection::moduleAdded(ProcessorBase *newModule) {
    auto module_section = std::make_unique<ModuleSection>(newModule->state, newModule->getAudioNodeDescriptor(),
        std::move (newModule->createEditor()), undo, audio_routing_manager_);
    module_section->setAreaSkinOverride(Skin::kFx);
    module_section->setDragAccentColor(Skin::kFXAccent);
    module_section->height = 300;
    module_section->onDragMove = [this](ModuleSection* dragged, juce::Rectangle<int> bounds) {
        updateDragSession(dragged, bounds);
    };
    module_section->onDragEnd = [this](ModuleSection* dragged, juce::Rectangle<int>) {
        endDragSession(dragged);
    };

    { juce::ScopedLock lock(open_gl_critical_section_);
        container_->addSubSection(module_section.get());
    }
    module_section->applySkinFromTopLevel();
    module_section->setInterceptsMouseClicks(true, true);
    parentHierarchyChanged();
    //int height_to_add  = module_section->height;
    module_sections.emplace_back(std::move(module_section));


    if (!getLocalBounds().isEmpty()) {
        //this->setSize(getWidth(),getHeight() + height_to_add);
        resized();
    }

    for (auto listener: listeners_) {
        listener->added();
    }
    auto interface = findParentComponentOfClass<SynthGuiInterface>();
    for (auto sub : sub_sections_) {
            OpenGlComponent::setScissorBounds(sub, viewport_.getLocalBounds(), *interface->getOpenGlWrapper());
            for (auto slider : sub->all_sliders_) {
                //slider.second->setScissor(this, open_gl);
                slider.second->setScissorComponent(&viewport_);
            }
        }
        container_->setScissorComponent(&viewport_);
        for (auto component : container_->open_gl_components_) {
            component->setScissorComponent(&viewport_);
        }
        for (auto sub : container_->sub_sections_) {
            for (auto view : sub->sub_sections_) {
                for (auto component : view->open_gl_components_) {
                    component->setScissorComponent(&viewport_);
                }
            }
            for (auto component : sub->open_gl_components_) {
                component->setScissorComponent(&viewport_);
            }
        }

}
void EffectModuleSection::resized() {
    //ModulesInterface::resized();
    static constexpr float kEffectOrderWidthPercent = 0.2f;
    static constexpr int kHeaderSidePadding = 6;
    static constexpr int kHeaderControlGap = 4;
    static constexpr int kRoutingControlHeight = 14;

    static auto button_size = static_cast<int>(findValue(Skin::kAddButtonSize));
    static int kHeaderControlsHeight = button_size;

    ScopedLock lock(open_gl_critical_section_);

    const int title_width = static_cast<int>(getTitleWidth());

    int order_width = getWidth() * kEffectOrderWidthPercent;
    //    effect_order_->setBounds(0, 0, order_width, getHeight());
    //    effect_order_->setSizeRatio(size_ratio_);
    int large_padding = findValue(Skin::kLargePadding);
    int shadow_width = getComponentShadowWidth();
    int viewport_x = 0 + large_padding - shadow_width;
    int viewport_width = getWidth() - viewport_x - large_padding + 2 * shadow_width;
    auto area = getLocalBounds();
    auto header = area.removeFromTop(30);
    toggle_button_->setBounds(0,0,getTitleWidth(),getTitleWidth());

    if (isExpanded()) {
        viewport_.setVisible(true);
        container_->setVisible(true);
        viewport_.setBounds(0,getTitleWidth(),getWidth(),getHeight()-getTitleWidth()-2);
        setEffectPositions();
        // scroll_bar_->setBounds(getWidth() - large_padding, title_width + large_padding, large_padding - 2, std::max(0, getHeight() - title_width - (large_padding + 2 * shadow_width)));

        // Match the shared audio-chain scrollbar instead of using the FX accent.
        // The look-and-feel stores the global skin values, so this remains theme-aware
        // without resolving through this section's red Skin::kFx override.
        scroll_bar_->setColor(getLookAndFeel().findColour(Skin::kWidgetPrimary1));

        // Clip every live child to the FX viewport while scrolling, matching
        // SoundModuleSection::resized(). Without this, scrolled FX child content is not
        // bounded to the visible viewport and bleeds into the section below.
        container_->setScissorComponent(&viewport_);
        for (auto component : container_->open_gl_components_)
            component->setScissorComponent(&viewport_);
        for (auto sub : container_->sub_sections_) {
            sub->setScissorComponent(&viewport_);
            for (auto slider : sub->all_sliders_)
                slider.second->setScissorComponent(&viewport_);
            for (auto component : sub->open_gl_components_)
                component->setScissorComponent(&viewport_);
            for (auto view : sub->sub_sections_) {
                view->setScissorComponent(&viewport_);
                for (auto slider : view->all_sliders_)
                    slider.second->setScissorComponent(&viewport_);
                for (auto component : view->open_gl_components_)
                    component->setScissorComponent(&viewport_);
            }
        }
    }
    else
    {
        viewport_.setVisible(false);
        container_->setVisible(false);
        viewport_.setBounds(0,0,0,0);
        container_->setBounds(0,0,0,0);
    }

    SynthSection::resized();
    redoBackgroundImage();
    //footer_body->setBounds(0,getHeight()-1, getWidth(), getTitleWidth());
    footer_body->setRounding(findValue(Skin::kBodyRounding));
    footer_body->setColor(findColour(Skin::kBody, true));

    header_body_->setBounds(0, 0, getWidth(), title_width);
    auto header_bounds = header_body_->getBounds();
    header_body_->setColor(findColour(Skin::kBodyHeading, true));

    header_title_->setBounds(header_bounds);
    header_title_->setText(getName());
    header_title_->setTextSize(size_ratio_ * 14.0f);
    header_title_->setColor(findColour(Skin::kHeadingText, true));


    const int controls_y = title_width;
    const int controls_height = kHeaderControlsHeight;
    const int available_width = std::max(0, getWidth() - 2 * kHeaderSidePadding);
    const int gap = available_width > button_size ? kHeaderControlGap : 0;
    const int combo_width = std::max(0, available_width - button_size - gap);
    const int control_height = std::min(kRoutingControlHeight, controls_height);
    const int control_y = controls_y + (controls_height - control_height) / 2;

    // routing_combo_box_->setBounds(kHeaderSidePadding, control_y, combo_width, control_height);

    add_effect_button_background_->setBounds(add_effect_button_->getBounds().reduced(4));
    add_effect_button_->setColour(Skin::kIconButtonOff, findColour(Skin::kIconButtonOff, true));
    add_effect_button_->setColour(Skin::kIconButtonOffHover, findColour(Skin::kIconButtonOffHover, true));
    add_effect_button_->setColour(Skin::kIconButtonOffPressed, findColour(Skin::kIconButtonOffPressed, true));

    const auto border_bounds = getLocalBounds();
    const auto border_color = findColour(Skin::kBorder, true);
    // The rounded-border shader's zero-radius inner mask has non-zero alpha across the
    // quad interior. A half-unit floor is the shader's antialias cutoff and keeps a
    // square-looking zero-radius skin border transparent away from its perimeter.
    const auto border_rounding = std::max(0.5f, findValue(Skin::kBodyRounding));
    for (auto* border : { border_overlay_.get(), border_overlay_second_pass_.get() }) {
        border->setBounds(border_bounds);
        border->setColor(border_color);
        border->setRounding(border_rounding);
        border->setThickness(1.0f, true);
    }

    static constexpr int kLanePortSize = 24;
    static constexpr int kLanePortInset = 6;
    static constexpr int kConnectionSlotSpacing = 2;

    const int port_y = (title_width - kLanePortSize) / 2;

    if (lane_input_port_) {
        lane_input_port_->setBounds(kLanePortInset, port_y, kLanePortSize, kLanePortSize);

        lane_input_slots_->setBounds(
            lane_input_port_->getRight() + kConnectionSlotSpacing,
            lane_input_port_->getY(),
            AudioConnectionSlots::kPreferredWidth,
            lane_input_port_->getHeight());
    }

    if (lane_output_port_) {
        lane_output_port_->setBounds(getWidth() - kLanePortInset - kLanePortSize,
            port_y, kLanePortSize, kLanePortSize);

        lane_output_slots_->setBounds(
            lane_output_port_->getX()
                - kConnectionSlotSpacing
                - AudioConnectionSlots::kPreferredWidth,
            lane_output_port_->getY(),
            AudioConnectionSlots::kPreferredWidth,
            lane_output_port_->getHeight());
    }
}

void EffectModuleSection::removeModule(ProcessorBase *newModule) {
    // Find exactly the one module whose state matches. find_if (vs non-stable
    // std::partition) does not reorder the surviving modules.
    auto it = [&]() {
        juce::ScopedLock lock(this->open_gl_critical_section_);
        return std::find_if(module_sections.begin(), module_sections.end(),
                            [newModule](auto& section) {
                                return section->state == newModule->state;
                            });
    }();

    // Guard: if not found (e.g. double-removal), dereferencing end() is UB.
    if (it == module_sections.end())
        return;

    ModuleSection* section = it->get();

    // A removal mid-drag (undo/redo or external state change) must not leave stale
    // drag visuals or a dangling dragged-module pointer.
    if (dragged_module_ != nullptr)
        clearDragSession();

    // Make invisible before any rebake: paintChildrenBackgrounds skips invisible
    // children, so the removed module is excluded from the scroll background.
    section->setVisible(false);

    // Move ownership out of module_sections into a strong keep-alive, then erase the
    // slot and remove the subsection from the container -- all on the message thread
    // under open_gl_critical_section_, so container_->sub_sections_ and the slider maps
    // are mutated consistently with the renderer/resized() (which hold the same lock).
    // removeSubSection is CPU-only (no GL calls).
    std::shared_ptr<ModuleSection> keep_alive;
    {
        juce::ScopedLock lock(this->open_gl_critical_section_);
        keep_alive = std::shared_ptr<ModuleSection>(std::move(*it));
        module_sections.erase(it);
        this->container_->removeSubSection(keep_alive.get());
    }

    // Notify listeners (modulation rebuild is deferred/coalesced) and reflow. The
    // container is already consistent, so resized()/redoBackgroundImage see valid state.
    for (auto listener : listeners_)
        listener->removed();
    resized();

    // Async GL-only cleanup: free GL resources on the GL thread (non-blocking, so the
    // message thread is never parked), then drop the keep-alive back on the message
    // thread so ~ModuleSection() (a JUCE Component) runs there. The lambda captures
    // keep_alive only and does NOT mutate container_/sub_sections_.
    auto *_parent = findParentComponentOfClass<SynthGuiInterface>();
    _parent->getOpenGlWrapper()->context.executeOnGLThread([keep_alive](juce::OpenGLContext &openGLContext) {
        keep_alive->destroyOpenGlComponents(openGLContext);
        juce::MessageManager::callAsync([keep_alive]() mutable {
            keep_alive.reset();
        });
    }, false);
}

void EffectModuleSection::moduleListChanged() {
}

int EffectModuleSection::indexOfModuleSection(const ModuleSection* section) const {
    for (int i = 0; i < (int)module_sections.size(); ++i) {
        if (module_sections[i].get() == section)
            return i;
    }
    return -1;
}

void EffectModuleSection::beginDragSession(ModuleSection* dragged) {
    dragged_module_ = dragged;
    drop_target_module_ = nullptr;
    dragged->setAlwaysOnTop(true);
    // Keep the dragged module out of the baked scroll image: its live GL content and
    // ModuleSection body-fill quad represent it while it floats.
    container_->setBakeExcludedChild(dragged);

    for (auto& section : module_sections)
        section->setDragVisual(section.get() == dragged ? ModuleSection::DragVisual::kDragged
                                                        : ModuleSection::DragVisual::kDimmed);

    insertion_region_->setColor(findColour(Skin::kFXAccent, true));
    insertion_region_->setRounding(std::max(0.5f, findValue(Skin::kBodyRounding)));
    insertion_region_->setAlpha(0.18f);

    // Boundary bands share the insertion region's accent and translucency.
    drag_boundary_bands_->setColor(findColour(Skin::kFXAccent, true));
    drag_boundary_bands_->setAlpha(0.18f);

    // Keep mouseDrag firing while the pointer rests near a viewport edge so
    // autoScroll continues without pointer movement.
    juce::Component::beginDragAutoRepeat(16);

    setEffectPositions();
    redoBackgroundImage();
}

void EffectModuleSection::updateDragSession(ModuleSection* dragged, juce::Rectangle<int> bounds) {
    if (dragged_module_ == nullptr)
        beginDragSession(dragged);
    if (dragged != dragged_module_)
        return;

    // Scroll when the pointer nears the top/bottom of the viewport. All reorder
    // geometry below is container-relative, so scrolling needs no compensation.
    // A generous activation border suits these tall modules; autoScroll also keeps
    // scrolling while the pointer is past the viewport edge (e.g. over the footer).
    const auto mouse_in_viewport = viewport_.getMouseXYRelative();
    viewport_.autoScroll(mouse_in_viewport.x, mouse_in_viewport.y, 40, 8);

    int index = indexOfModuleSection(dragged);
    if (index < 0)
        return;

    // Edge-crossing reorder: the dragged module pushes neighbors out of the way.
    // Layout is refreshed after each swap so the next comparison uses live bounds.
    // The margin adds hysteresis: for similar-height modules the swap-down and
    // swap-back thresholds otherwise land on the same pixel and 1px of jitter
    // (e.g. scroll rounding) makes neighbors flutter between slots.
    static constexpr int kSwapHysteresis = 8;
    bool moved = false;
    while (index + 1 < (int)module_sections.size()
           && bounds.getBottom() > module_sections[index + 1]->getBounds().getCentreY() + kSwapHysteresis) {
        std::swap(module_sections[index], module_sections[index + 1]);
        ++index;
        moved = true;
        setEffectPositions();
    }
    while (index > 0
           && bounds.getY() < module_sections[index - 1]->getBounds().getCentreY() - kSwapHysteresis) {
        std::swap(module_sections[index], module_sections[index - 1]);
        --index;
        moved = true;
        setEffectPositions();
    }
    if (moved)
        redoBackgroundImage();

    // The neighbor currently being overlapped (the next module to be displaced)
    // gets the subtle drop-target accent.
    ModuleSection* target = nullptr;
    if (index + 1 < (int)module_sections.size()
        && bounds.getBottom() > module_sections[index + 1]->getBounds().getY())
        target = module_sections[index + 1].get();
    else if (index > 0
             && bounds.getY() < module_sections[index - 1]->getBounds().getBottom())
        target = module_sections[index - 1].get();

    if (target != drop_target_module_) {
        if (drop_target_module_ != nullptr)
            drop_target_module_->setDragVisual(ModuleSection::DragVisual::kDimmed);
        if (target != nullptr)
            target->setDragVisual(ModuleSection::DragVisual::kDropTarget);
        drop_target_module_ = target;
    }
}

void EffectModuleSection::endDragSession(ModuleSection* dragged) {
    if (dragged != dragged_module_) {
        clearDragSession();
        return;
    }

    const int ui_index = indexOfModuleSection(dragged);
    clearDragSession();

    // Persist the previewed order: exactly one ValueTree move per drop, through the
    // undo manager. The resulting moduleOrderChanged() callback re-syncs
    // module_sections to tree order (a no-op sort here, but it also serves undo/redo).
    // NOTE: DSP processing order is intentionally not updated yet (deferred); the
    // audible chain follows tree order again after preset/state reload.
    juce::ValueTree parent_tree = dragged->state.getParent();
    const int tree_index = parent_tree.indexOf(dragged->state);
    if (ui_index < 0 || tree_index < 0)
        return;

    // Map the desired UI slot to a raw child index in the parent tree. Anchoring on
    // the following module's state keeps this correct even if the tree ever holds
    // non-module children.
    int new_tree_index;
    if (ui_index + 1 < (int)module_sections.size()) {
        const int next_raw = parent_tree.indexOf(module_sections[ui_index + 1]->state);
        if (next_raw < 0)
            return;
        new_tree_index = tree_index < next_raw ? next_raw - 1 : next_raw;
    }
    else
        new_tree_index = parent_tree.getNumChildren() - 1;

    if (new_tree_index != tree_index) {
        undo.beginNewTransaction();
        list.moveChild(tree_index, new_tree_index, &undo);
    }
}

void EffectModuleSection::clearDragSession() {
    for (auto& section : module_sections)
        section->setDragVisual(ModuleSection::DragVisual::kNormal);
    if (dragged_module_ != nullptr)
        dragged_module_->setAlwaysOnTop(false);
    container_->setBakeExcludedChild(nullptr);
    dragged_module_ = nullptr;
    drop_target_module_ = nullptr;
    insertion_region_->setAlpha(0.0f);
    drag_boundary_bands_->setAlpha(0.0f);
    drag_boundary_bands_->setNumQuads(0);
    juce::Component::beginDragAutoRepeat(0);

    setEffectPositions();
    redoBackgroundImage();
}

void EffectModuleSection::moduleOrderChanged() {
    {
        juce::ScopedLock lock(open_gl_critical_section_);
        std::stable_sort(module_sections.begin(), module_sections.end(),
                         [](const auto& a, const auto& b) {
                             juce::ValueTree parent = a->state.getParent();
                             return parent.indexOf(a->state) < parent.indexOf(b->state);
                         });
    }
    setEffectPositions();
    redoBackgroundImage();
}
// void EffectModuleSection::renderOpenGlComponents(OpenGlWrapper &open_gl, bool animate) {
//     ScopedLock lock(open_gl_critical_section_);
//
//     OpenGlComponent::setViewPort(&viewport_, open_gl);
//
//     float image_width = background_.getImageWidth(); //electrosynth::utils::nextPowerOfTwo(background_.getImageWidth());
//     float image_height =background_.getImageHeight(); // electrosynth::utils::nextPowerOfTwo(background_.getImageHeight());
//     int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;// getPixelMultiple();
//     float width_ratio = image_width / (container_->getWidth() * mult);
//     float height_ratio = image_height / (viewport_.getHeight() * mult);
//    // DBG(viewport_.getViewPositionY());
//     float y_offset =(2.0f * viewport_.getViewPositionY()) /viewport_.getHeight();
//
//     // --- Debug output ---
//     // DBG("image_width: " + juce::String(image_width));
//     // DBG("image_height: " + juce::String(image_height));
//     // DBG("mult (scale factor): " + juce::String(mult));
//     // DBG("container width: " + juce::String(container_->getWidth()));
//     // DBG("viewport height: " + juce::String(viewport_.getHeight()));
//     // DBG("width_ratio: " + juce::String(width_ratio));
//     // DBG("height_ratio: " + juce::String(height_ratio));
//     // DBG("viewport Y offset: " + juce::String(viewport_.getViewPositionY()));
//     // DBG("computed y_offset: " + juce::String(y_offset));
//     //
//     background_.setTopLeft(-1.0f, 1.0f+ y_offset);
//     background_.setTopRight(-1.0f + 2.0f * width_ratio,  1.0f+y_offset);
//     background_.setBottomLeft(-1.0f, 1.0f - 2.0f * height_ratio + y_offset);
//     background_.setBottomRight(-1.0f + 2.0f * width_ratio, 1.0f - 2.0f * height_ratio + y_offset);
//     background_.setColor(Colours::white);
//     background_.drawImage(open_gl);
//
//     OpenGlComponent::setScissorBounds(this, getLocalBounds(),open_gl);
//     //TODO: clean up. this is to check here becuase I can do this creationlazy do better
//     for (auto sub : sub_sections_) {
//         OpenGlComponent::setScissorBounds(sub, viewport_.getLocalBounds(), open_gl);
//         for (auto slider : sub->all_sliders_) {
//             //slider.second->setScissor(this, open_gl);
//             slider.second->setScissorComponent(&viewport_);
//         }
//         for (auto component : sub->open_gl_components_) {
//             component->setScissorComponent(&viewport_);
//         }
//     }
//     container_->setScissorComponent(&viewport_);
//     for (auto component : container_->open_gl_components_) {
//         component->setScissorComponent(&viewport_);
//     }
//     for (auto sub : container_->sub_sections_) {
//         for (auto view : sub->sub_sections_) {
//             for (auto component : view->open_gl_components_) {
//                 component->setScissorComponent(&viewport_);
//             }
//         }
//         for (auto component : sub->open_gl_components_) {
//             component->setScissorComponent(&viewport_);
//         }
//     }
// }

// void EffectModuleSection::mouseDown(const juce::MouseEvent &e) {
//     // mouse_down_y_ = e.y;
//     //
//     // for (int i =0; i< module_sections.size(); ++i) {
//     //     if ( module_sections[i]->hover_) {
//     //         currently_dragged_ = module_sections[i].get();
//     //         last_dragged_index_ =i;
//     //     }
//     //
//     // }
//     // dragged_starting_y_ = currently_dragged_->getY();
//     // currently_dragged_->setAlwaysOnTop(true);
// }

// void EffectModuleSection::mouseDrag(const MouseEvent& e) {
//     if (currently_dragged_ == nullptr)
//         return;
//
//     int delta_y = e.y - mouse_down_y_;
//     int clamped_y = electrosynth::utils::iclamp(dragged_starting_y_ + delta_y, 0,
//                                          getHeight() - currently_dragged_->getHeight());
//     currently_dragged_->setTopLeftPosition(currently_dragged_->getX(), clamped_y);
//
//     int next_index = ;
//     if (next_index != last_dragged_index_) {
//         moveEffect(last_dragged_index_, next_index);
//         last_dragged_index_ = next_index;
//     }
void EffectModuleSection::redoBackgroundImage() {
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    Colour background = findColour(Skin::kBackground, true);
    int height = std::max(container_->getHeight(), static_cast<int>(viewport_.getHeight()));
    if (height == 0)
        height = getHeight();
    int width = std::max(container_->getWidth(), getWidth());
    int mult = juce::Desktop::getInstance().getDisplays().getDisplayForRect(getScreenBounds())->scale;

    Image background_image = Image(Image::ARGB, width * mult, height * mult, true);

    Graphics background_graphics(background_image);
    background_graphics.addTransform(AffineTransform::scale(mult));
    background_graphics.fillAll(background);
    if (isExpanded())
        container_->paintBackground(background_graphics);
    background_graphics.setColour(juce::Colours::aliceblue);
    background_graphics.fillRect(juce::Rectangle<float>(0.0f, 0.0f, 1.0f, (float)height));
    background_graphics.fillRect(juce::Rectangle<float>((float)width - 1.0f, 0.0f, 1.0f, (float)height));
    background_.setOwnImage(background_image);
}

void EffectModuleSection::paintBackground(Graphics &g) {
    paintBody(g);
    paintBorder(g);
    redoBackgroundImage();
}
