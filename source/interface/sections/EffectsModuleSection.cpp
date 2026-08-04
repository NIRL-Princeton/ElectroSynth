//
// Created by Davis Polito on 11/19/24.
//

#include "EffectsModuleSection.h"
#include "ModuleSection.h"
#include "synth_gui_interface.h"
#include "Processors/ProcessorBase.h"
#include "modulation_manager.h"
#include "synth_base.h"
#include "EffectList.h"
#include "FxDragCoordinator.h"

namespace {
// Drag-mode boundary bands between adjacent non-dragged modules: pool size and
// band height (centered on the shared module edge, in logical units).
constexpr int kMaxDragBoundaryBands = 16;
constexpr int kDragBoundaryBandHeight = 8;
constexpr float kExternalTransferDimAlpha = 0.38f;
constexpr float kInsertionPreviewAlpha = 0.18f;

juce::String getEffectTypeDisplayName(const juce::String& type) {
    // UI labels are kept separate from persistent type tokens. Adding a future
    // effect only requires one entry here; unknown tokens still receive a usable
    // title rather than retaining the processor UUID-based editor name.
    static const std::map<juce::String, juce::String> display_names {
        { "filt", "Filter" },
        { "delay", "Delay" },
    };

    if (const auto found = display_names.find(type); found != display_names.end())
        return found->second;

    auto fallback = type.replaceCharacters("_-", "  ").trim();
    if (fallback.isEmpty())
        return "Effect";
    return fallback.substring(0, 1).toUpperCase() + fallback.substring(1);
}

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

EffectModuleSection::EffectModuleSection(ModulationManager *m, EffectList &module_list,const juce::ValueTree &v, juce::UndoManager& um) :
ModulesInterface( module_list), footer_body(new OpenGlQuad(Shaders::kRoundedRectangleFragment)), state(v), undo(um)
{
    scroll_bar_ = std::make_unique<OpenGlScrollBar>();
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

    // Intent-only content overlays. They never cover the fixed header. The dim pass
    // is added first so the armed lane's pale highlight remains readable above it.
    external_transfer_dim_ = std::make_shared<OpenGlQuad>(
        Shaders::kRoundedRectangleFragment, "effect_external_transfer_dim");
    external_transfer_dim_->setInterceptsMouseClicks(false, false);
    external_transfer_dim_->setAlwaysOnTop(true);
    external_transfer_dim_->setColor(juce::Colours::black);
    external_transfer_dim_->setAlpha(0.0f, true);
    addOpenGlComponent(external_transfer_dim_);

    external_transfer_highlight_ = std::make_shared<OpenGlQuad>(
        Shaders::kRoundedRectangleFragment, "effect_external_transfer_highlight");
    external_transfer_highlight_->setInterceptsMouseClicks(false, false);
    external_transfer_highlight_->setAlwaysOnTop(true);
    external_transfer_highlight_->setAlpha(0.0f, true);
    addOpenGlComponent(external_transfer_highlight_);

    toggle_button_->setVisible(false);
    setInterceptsMouseClicks(true,true);

    setSkinOverride(Skin::kFx);
}

EffectModuleSection::~EffectModuleSection() {
   if (drag_coordinator_ != nullptr)
       drag_coordinator_->unregisterLane(*this);
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

    // Keep FX modules inside the lane border and use the same skin metric between
    // modules. All drag/drop geometry below derives from these bounds.
    const int module_padding = std::max(0, static_cast<int>(findValue(Skin::kPadding)));
    const int module_x = module_padding;
    const int module_width = std::max(0, viewport_.getWidth() - 2 * module_padding);
    int start_y = module_padding;
    juce::Rectangle<int> gap_bounds;
    // Boundaries between two adjacent non-dragged modules; edges touching the
    // insertion gap are excluded (the gap region itself marks those).
    std::vector<int> boundary_ys;
    bool previous_was_module = false;

    std::map<juce::String, int> type_counts;
    int laid_out_module_count = 0;
    for (int i = 0; i < module_sections.size(); ++i) {
        ModuleSection* section = module_sections[i].get();
        const auto type = section->state.getProperty(IDs::type).toString();
        section->setName(getEffectTypeDisplayName(type) + " "
                         + juce::String(++type_counts[type]));

        // Once a destination is entered, the source wrapper contributes no height.
        if (section == dragged_module_ && external_source_excluded_) {
            previous_was_module = false;
            continue;
        }

        if (external_target_preview_active_
            && laid_out_module_count == external_target_insertion_index_) {
            gap_bounds = { module_x, start_y, module_width, external_target_gap_height_ };
            start_y += external_target_gap_height_ + module_padding;
            previous_was_module = false;
        }

        // During a same-lane drag the module floats at its pointer-driven position;
        // its slot in the stack stays open as the insertion gap.
        if (section == dragged_module_) {
            gap_bounds = { module_x, start_y, module_width, section->height };
            start_y += section->height + module_padding;
            previous_was_module = false;
            continue;
        }

        if (previous_was_module)
            boundary_ys.push_back(start_y - module_padding / 2);
        previous_was_module = true;

        // Size each FX module from its contained view's dynamic row layout instead of the
        // full viewport height. Set bounds once first so the FX view gets its width and can
        // compute a width-dependent row count, then read the preferred height and apply it.
        // (FX-local: SoundModuleSection sizes its modules separately.)
        section->setBounds(module_x, start_y, module_width, section->height);
        section->height = section->getPreferredHeight();
        section->setDrawBottomSeparator(i + 1 < module_sections.size());
        section->setBounds(module_x, start_y, module_width, section->height);
        start_y += section->height + module_padding;
        ++laid_out_module_count;
    }

    if (external_target_preview_active_
        && external_target_insertion_index_ == laid_out_module_count) {
        gap_bounds = { module_x, start_y, module_width, external_target_gap_height_ };
        start_y += external_target_gap_height_ + module_padding;
    }

    // Callista footer: keep the add affordance in the scrollable FX content after the
    // final module instead of consuming fixed header space.
    const int button_size = std::max(0, static_cast<int>(findValue(Skin::kAddButtonSize)));
    const int footer_top_gap = 10;
    const int footer_bottom_padding = std::max(10, module_padding);
    const int footer_y = start_y;
    const int button_y = footer_y + footer_top_gap;
    const int footer_height = footer_top_gap + button_size + footer_bottom_padding;
    const int button_x = std::max(0, (viewport_.getWidth() - button_size) / 2);

    footer_body->setBounds(0, footer_y, viewport_.getWidth(), footer_height);
    add_effect_button_->setBounds(button_x, button_y, button_size, button_size);
    add_effect_button_background_->setBounds(add_effect_button_->getBounds().reduced(4));

    // setSize (not setBounds): the container's origin belongs to the viewport's scroll
    // logic; any explicit origin here is overwritten by setViewPosition.
    container_->setSize(viewport_.getWidth(), footer_y + footer_height);
    viewport_.setViewPosition(view_position);

    if (insertion_region_ != nullptr) {
        if ((dragged_module_ != nullptr || external_target_preview_active_) && !gap_bounds.isEmpty()) {
            insertion_region_->setBounds(gap_bounds);  // container coordinates
            // Layout may temporarily have no gap while a source-owned wrapper is
            // excluded and then re-entered as an external target. Reassert alpha
            // whenever the gap is valid so a prior empty layout cannot leave the
            // source lane's red target permanently invisible.
            insertion_region_->setAlpha(kInsertionPreviewAlpha);
        }
        else
            insertion_region_->setAlpha(0.0f);
    }

    if (drag_boundary_bands_ != nullptr) {
        const int num_bands = std::min((int)boundary_ys.size(), kMaxDragBoundaryBands);
        const bool show_same_lane_bands = dragged_module_ != nullptr
                                       && !external_source_excluded_;
        if ((show_same_lane_bands || external_target_preview_active_)
            && num_bands > 0 && start_y > 0) {
            // The multi-quad spans the whole container; each band is placed in the
            // quad's normalized GL space (y up, so the component top maps to +1).
            drag_boundary_bands_->setBounds(0, 0, viewport_.getWidth(), start_y);
            drag_boundary_bands_->setNumQuads(num_bands);
            const float total_height = (float)start_y;
            const float band_height_gl = 2.0f * kDragBoundaryBandHeight / total_height;
            const float lane_width = static_cast<float>(std::max(1, viewport_.getWidth()));
            const float band_left_gl = -1.0f + 2.0f * module_x / lane_width;
            const float band_width_gl = 2.0f * module_width / lane_width;
            for (int i = 0; i < num_bands; ++i) {
                const float band_bottom = boundary_ys[i] + kDragBoundaryBandHeight / 2.0f;
                drag_boundary_bands_->setQuad(i, band_left_gl,
                                              1.0f - 2.0f * band_bottom / total_height,
                                              band_width_gl, band_height_gl);
            }
        }
        else
            drag_boundary_bands_->setNumQuads(0);
    }
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
        std::move (newModule->createEditor()), undo);
    module_section->setAreaSkinOverride(Skin::kFx);
    module_section->setDragAccentColor(Skin::kFXAccent);
    module_section->height = 300;
    configureModuleSectionDragCallbacks(*module_section);

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
            for (auto component : sub->open_gl_components_) {
                component->setScissorComponent(&viewport_);
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

void EffectModuleSection::configureModuleSectionDragCallbacks(ModuleSection& module) {
    module.onDragStart = [this](ModuleSection* dragged, juce::Point<int> screen) {
        if (drag_coordinator_ != nullptr)
            drag_coordinator_->dragStarted(*this, *dragged, screen);
    };
    module.onDragMove = [this](ModuleSection* dragged, juce::Rectangle<int> bounds,
                                        juce::Point<int> screen) {
        if (drag_coordinator_ != nullptr)
            drag_coordinator_->dragMoved(*dragged, screen);
        if (drag_coordinator_ == nullptr || !drag_coordinator_->isExternalPreviewActive(dragged))
            updateDragSession(dragged, bounds);
    };
    module.onDragEnd = [this](ModuleSection* dragged, juce::Rectangle<int>,
                                       juce::Point<int> screen) {
        if (drag_coordinator_ != nullptr && drag_coordinator_->ownsDrag(dragged))
            drag_coordinator_->dragEnded(*dragged, screen);
        else if (dragged_module_ == dragged)
            endDragSession(dragged);
    };
}

bool EffectModuleSection::transferModuleTo(EffectModuleSection& target,
                                           ProcessorBase* processor,
                                           int targetEffectIndex) {
    if (processor == nullptr || &target == this)
        return false;

    const auto found = std::find_if(module_sections.begin(), module_sections.end(),
                                    [processor](const auto& section) {
                                        return section != nullptr
                                            && section->state == processor->state;
                                    });
    if (found == module_sections.end())
        return false;

    auto transferred = std::move(*found);
    module_sections.erase(found);
    auto* raw = transferred.get();

    {
        juce::ScopedLock sourceLock(open_gl_critical_section_);
        container_->setBakeExcludedChild(nullptr);
        container_->removeSubSection(raw);
    }

    dragged_module_ = nullptr;
    drop_target_module_ = nullptr;
    external_source_excluded_ = false;
    external_target_preview_active_ = false;
    external_target_insertion_index_ = -1;
    external_target_gap_height_ = 0;
    external_gap_move_centre_y_ = 0;
    external_gap_move_direction_ = 0;
    insertion_region_->setAlpha(0.0f);
    drag_boundary_bands_->setAlpha(0.0f);
    drag_boundary_bands_->setNumQuads(0);
    juce::Component::beginDragAutoRepeat(0);

    target.clearExternalTargetPreview();
    target.configureModuleSectionDragCallbacks(*raw);
    const int insertionIndex = juce::jlimit(0,
        static_cast<int>(target.module_sections.size()), targetEffectIndex);
    target.module_sections.insert(target.module_sections.begin() + insertionIndex,
                                  std::move(transferred));
    {
        juce::ScopedLock targetLock(target.open_gl_critical_section_);
        target.container_->addSubSection(raw);
    }
    raw->setVisible(true);
    raw->setInterceptsMouseClicks(true, true);
    raw->setHorizontalDragOwnedExternally(false);
    raw->setExternallyVisualHosted(false);
    raw->setAlwaysOnTop(false);
    raw->setDragVisual(ModuleSection::DragVisual::kNormal);
    target.setModuleDragScissor(*raw, &target.viewport_);
    raw->applySkinFromTopLevel();

    setEffectPositions();
    target.setEffectPositions();
    redoBackgroundImage();
    target.redoBackgroundImage();
    for (auto* listener : listeners_)
        listener->removed();
    for (auto* listener : target.listeners_)
        listener->added();
    return true;
}
void EffectModuleSection::resized() {
    //ModulesInterface::resized();
    static constexpr float kEffectOrderWidthPercent = 0.2f;

    ScopedLock lock(open_gl_critical_section_);

    const int title_width = static_cast<int>(getTitleWidth());
    const int header_height = title_width;

    int order_width = getWidth() * kEffectOrderWidthPercent;
    //    effect_order_->setBounds(0, 0, order_width, getHeight());
    //    effect_order_->setSizeRatio(size_ratio_);
    auto area = getLocalBounds();
    auto header = area.removeFromTop(30);
    toggle_button_->setBounds(0,0,getTitleWidth(),getTitleWidth());
    if (isExpanded()) {
        viewport_.setVisible(true);
        container_->setVisible(true);
        viewport_.setBounds(0, header_height, getWidth(),
                            std::max(0, getHeight() - header_height - 2));
        setEffectPositions();
        // The external scrollbar remains connected to the viewport for range/state
        // synchronization but is intentionally not added to the visible component or
        // OpenGL trees. Wheel/trackpad scrolling continues through the viewport.

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
    //ooter_body->setBounds(0,getHeight()-1, getWidth(), getTitleWidth());
    footer_body->setRounding(findValue(Skin::kBodyRounding));
    footer_body->setColor(findColour(Skin::kBody, true));

    header_body_->setBounds(0, 0, getWidth(), header_height);
    header_body_->setColor(findColour(Skin::kBodyHeading, true));
    header_title_->setBounds(0, 0, getWidth(), title_width);
    header_title_->setText(getName());
    header_title_->setTextSize(size_ratio_ * 14.0f);
    header_title_->setColor(findColour(Skin::kHeadingText, true));

    external_transfer_highlight_->setBounds(viewport_.getBounds());
    external_transfer_highlight_->setColor(findColour(Skin::kLightenScreen, true));
    external_transfer_highlight_->setRounding(
        std::max(0.5f, findValue(Skin::kBodyRounding)));
    external_transfer_highlight_->setScissorComponent(&viewport_);
    external_transfer_dim_->setBounds(viewport_.getBounds());
    external_transfer_dim_->setRounding(
        std::max(0.5f, findValue(Skin::kBodyRounding)));
    external_transfer_dim_->setScissorComponent(&viewport_);

    routing_combo_box_->setBounds(0, 0, 0, 0);
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
    if (drag_coordinator_ != nullptr
        && drag_coordinator_->getPhase() != FxDragCoordinator::Phase::Idle)
        drag_coordinator_->cancelDrag();
    else if (dragged_module_ != nullptr)
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
    insertion_region_->setAlpha(kInsertionPreviewAlpha);

    // Boundary bands share the insertion region's accent and translucency.
    drag_boundary_bands_->setColor(findColour(Skin::kFXAccent, true));
    drag_boundary_bands_->setAlpha(kInsertionPreviewAlpha);

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

    updateVerticalReorderPreview(*dragged, bounds, false);
}

ModuleSection* EffectModuleSection::externalPreviewModuleAt(int logicalIndex) const {
    if (logicalIndex < 0)
        return nullptr;

    int current = 0;
    for (const auto& section : module_sections) {
        if (section.get() == dragged_module_ && external_source_excluded_)
            continue;
        if (current++ == logicalIndex)
            return section.get();
    }
    return nullptr;
}

void EffectModuleSection::updateVerticalReorderPreview(
    ModuleSection& dragged, juce::Rectangle<int> boundsInContainer, bool externalTarget,
    bool useOverlapDisplacement) {
    int gap_index = externalTarget ? external_target_insertion_index_
                                   : indexOfModuleSection(&dragged);
    if (gap_index < 0)
        return;

    const auto module_above = [this, externalTarget](int gap) -> ModuleSection* {
        if (externalTarget)
            return externalPreviewModuleAt(gap - 1);
        return gap > 0 ? module_sections[gap - 1].get() : nullptr;
    };
    const auto module_below = [this, externalTarget](int gap) -> ModuleSection* {
        if (externalTarget)
            return externalPreviewModuleAt(gap);
        return gap + 1 < static_cast<int>(module_sections.size())
                 ? module_sections[gap + 1].get() : nullptr;
    };

    // This is the ordinary same-lane center-crossing rule. External target preview
    // changes only how the gap index is stored; overlap, hysteresis, reflow, and
    // target highlighting are shared with the established vertical drag path.
    // Free target hover (useOverlapDisplacement) substitutes the ghost/module
    // vertical-intersection rule for center crossing: a neighbor is displaced when
    // the overlap reaches half the ghost's height or exceeds half of its own.
    static constexpr int kSwapHysteresis = 8;
    static constexpr int kOverlapReversalHysteresis = 12;
    const auto overlap_height = [&boundsInContainer](const ModuleSection* neighbor) {
        return std::min(boundsInContainer.getBottom(), neighbor->getBounds().getBottom())
             - std::max(boundsInContainer.getY(), neighbor->getBounds().getY());
    };
    const auto overlap_displaces = [&](const ModuleSection* neighbor) {
        const int overlap = overlap_height(neighbor);
        return 2 * overlap >= boundsInContainer.getHeight()
            || 2 * overlap > neighbor->getHeight();
    };
    // direction: +1 moves the gap down, -1 moves it up (container Y grows downward).
    const auto overlap_move_allowed = [&](int direction) {
        if (!useOverlapDisplacement)
            return true;
        if (external_gap_move_direction_ == 0 || external_gap_move_direction_ == direction)
            return true;
        const int travel = direction
                         * (boundsInContainer.getCentreY() - external_gap_move_centre_y_);
        return travel >= kOverlapReversalHysteresis;
    };
    const auto record_overlap_move = [&](int direction) {
        if (!useOverlapDisplacement)
            return;
        external_gap_move_centre_y_ = boundsInContainer.getCentreY();
        external_gap_move_direction_ = direction;
    };
    bool moved = false;
    while (auto* below = module_below(gap_index)) {
        if (useOverlapDisplacement
                ? !(overlap_displaces(below) && overlap_move_allowed(1))
                : boundsInContainer.getBottom()
                      <= below->getBounds().getCentreY() + kSwapHysteresis)
            break;
        record_overlap_move(1);
        if (externalTarget)
            external_target_insertion_index_ = ++gap_index;
        else {
            std::swap(module_sections[gap_index], module_sections[gap_index + 1]);
            ++gap_index;
        }
        moved = true;
        setEffectPositions();
    }
    while (auto* above = module_above(gap_index)) {
        if (useOverlapDisplacement
                ? !(overlap_displaces(above) && overlap_move_allowed(-1))
                : boundsInContainer.getY()
                      >= above->getBounds().getCentreY() - kSwapHysteresis)
            break;
        record_overlap_move(-1);
        if (externalTarget)
            external_target_insertion_index_ = --gap_index;
        else {
            std::swap(module_sections[gap_index], module_sections[gap_index - 1]);
            --gap_index;
        }
        moved = true;
        setEffectPositions();
    }
    if (moved)
        redoBackgroundImage();

    // The neighbor currently being overlapped (the next module to be displaced)
    // gets the subtle drop-target accent.
    ModuleSection* target = nullptr;
    if (auto* below = module_below(gap_index);
        below != nullptr && boundsInContainer.getBottom() > below->getBounds().getY())
        target = below;
    else if (auto* above = module_above(gap_index);
             above != nullptr && boundsInContainer.getY() < above->getBounds().getBottom())
        target = above;

    if (target != drop_target_module_) {
        if (drop_target_module_ != nullptr)
            drop_target_module_->setDragVisual(ModuleSection::DragVisual::kDimmed);
        if (target != nullptr)
            target->setDragVisual(ModuleSection::DragVisual::kDropTarget);
        drop_target_module_ = target;
    }
}

void EffectModuleSection::endDragSession(ModuleSection* dragged) {
    finishSameLaneDrag(*dragged, true);
}

void EffectModuleSection::finishSameLaneDrag(ModuleSection& dragged, bool commit) {
    if (&dragged != dragged_module_) {
        clearDragSession();
        return;
    }

    const int ui_index = indexOfModuleSection(&dragged);
    clearDragSession();

    if (!commit) {
        restoreTreeDerivedOrder();
        setEffectPositions();
        redoBackgroundImage();
        return;
    }

    // Persist the previewed order: exactly one ValueTree move per drop, through the
    // undo manager. The resulting moduleOrderChanged() callback re-syncs
    // module_sections to tree order (a no-op sort here, but it also serves undo/redo)
    // and publishes the identity-based DSP placement command.
    juce::ValueTree parent_tree = dragged.state.getParent();
    const int tree_index = parent_tree.indexOf(dragged.state);
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

void EffectModuleSection::restoreTreeDerivedOrder() {
    juce::ScopedLock lock(open_gl_critical_section_);
    std::stable_sort(module_sections.begin(), module_sections.end(),
                     [](const auto& a, const auto& b) {
                         const auto parent = a->state.getParent();
                         return parent.indexOf(a->state) < parent.indexOf(b->state);
                     });
}

juce::Rectangle<int> EffectModuleSection::getContentViewportScreenBounds() const {
    return viewport_.getScreenBounds();
}

juce::Rectangle<int> EffectModuleSection::getExternalHostedModuleScreenBounds(
    int draggedHeight, int pointerScreenY, int pointerOffsetY, int horizontalOffset) const {
    const auto viewport_screen = getContentViewportScreenBounds();
    const int module_padding = std::max(0, static_cast<int>(findValue(Skin::kPadding)));
    return { viewport_screen.getX() + module_padding + horizontalOffset,
             pointerScreenY - pointerOffsetY,
             std::max(0, viewport_screen.getWidth() - 2 * module_padding),
             std::max(1, draggedHeight) };
}

void EffectModuleSection::setExternalTransferHighlight(bool highlighted) {
    // kLightenScreen already carries the skin's intended translucent alpha. Do not
    // multiply it by another small value or the highlight becomes effectively invisible.
    external_transfer_highlight_->setAlpha(highlighted ? 1.0f : 0.0f);
}

void EffectModuleSection::setExternalTransferDimmed(bool dimmed) {
    external_transfer_dim_->setAlpha(dimmed ? kExternalTransferDimAlpha : 0.0f);
}

void EffectModuleSection::setModuleDragScissor(ModuleSection& module,
                                               juce::Component* scissor) {
    module.setScissorComponent(scissor);
    for (auto& slider : module.all_sliders_)
        slider.second->setScissorComponent(scissor);
    for (auto& component : module.open_gl_components_)
        component->setScissorComponent(scissor);
    for (auto* view : module.sub_sections_) {
        view->setScissorComponent(scissor);
        for (auto& slider : view->all_sliders_)
            slider.second->setScissorComponent(scissor);
        for (auto& component : view->open_gl_components_)
            component->setScissorComponent(scissor);
    }
}

void EffectModuleSection::beginExternalVisualHosting(
    ModuleSection& dragged, juce::Component& sharedContentClip) {
    if (dragged_module_ != &dragged)
        beginDragSession(&dragged);

    // The JUCE visual parent must move to the
    // shared clip: OpenGlComponent intersects visibility with every component
    // ancestor, so leaving it under the narrow source container clips an otherwise
    // correctly positioned external render to an empty rectangle. The source lane
    // retains unique_ptr/model/processor ownership throughout. Source layout remains
    // untouched until the hosted module's horizontal midpoint enters the target.
    const auto preserved_screen_bounds = dragged.getScreenBounds();
    dragged.setHorizontalDragOwnedExternally(true);
    dragged.setExternallyVisualHosted(true);
    dragged.setInterceptsMouseClicks(false, false);
    sharedContentClip.addAndMakeVisible(&dragged);
    dragged.setBounds(sharedContentClip.getLocalArea(nullptr, preserved_screen_bounds));
    setModuleDragScissor(dragged, &sharedContentClip);
}

void EffectModuleSection::restoreExternalVisualHosting(ModuleSection& dragged) {
    const auto preserved_screen_bounds = dragged.getScreenBounds();
    dragged.setExternallyVisualHosted(false);
    dragged.setInterceptsMouseClicks(true, true);
    dragged.setHorizontalDragOwnedExternally(false);
    container_->addAndMakeVisible(&dragged);
    auto restored_bounds = container_->getLocalArea(nullptr, preserved_screen_bounds);
    restored_bounds.setX(dragged.originalBounds.getX());
    dragged.setBounds(restored_bounds);
    setModuleDragScissor(dragged, &viewport_);
}

void EffectModuleSection::excludeExternalSourceFromLayout(ModuleSection& dragged) {
    // Resolve provisional same-lane swaps without touching the tree, then close the
    // source gap at the precise midpoint-snap transition.
    restoreTreeDerivedOrder();
    external_source_excluded_ = true;
    for (auto& section : module_sections)
        section->setDragVisual(section.get() == &dragged
                                   ? ModuleSection::DragVisual::kDragged
                                   : ModuleSection::DragVisual::kNormal);
    insertion_region_->setAlpha(0.0f);
    drag_boundary_bands_->setAlpha(0.0f);
    drag_boundary_bands_->setNumQuads(0);
    setEffectPositions();
    redoBackgroundImage();
}

void EffectModuleSection::updateExternalHostedModuleBounds(
    ModuleSection& dragged, juce::Rectangle<int> screenBounds,
    juce::Component& sharedContentClip) {
    setModuleDragScissor(dragged, &sharedContentClip);
    if (auto* parent = dragged.getParentComponent())
        dragged.setBounds(parent->getLocalArea(nullptr, screenBounds));
}

void EffectModuleSection::restoreExternalSourcePreview(ModuleSection& dragged) {
    external_source_excluded_ = false;
    restoreExternalVisualHosting(dragged);
    restoreTreeDerivedOrder();
    clearDragSession();
}

int EffectModuleSection::calculateExternalInsertionIndex(
    juce::Rectangle<int> boundsInContainer) const {
    // Initial gap placement only: count the laid-out modules whose centers sit
    // above the ghost's center. Subsequent gap movement is owned by the shared
    // overlap-displacement rule in updateVerticalReorderPreview.
    const int ghost_centre_y = boundsInContainer.getCentreY();
    int target_index = 0;
    for (const auto& section : module_sections) {
        if (section.get() == dragged_module_ && external_source_excluded_)
            continue;
        if (ghost_centre_y < section->getBounds().getCentreY())
            return target_index;
        ++target_index;
    }
    return target_index;
}

int EffectModuleSection::beginExternalTargetPreview(ModuleSection& dragged,
                                                    juce::Point<int> pointerScreen) {
    external_target_preview_active_ = true;
    external_target_gap_height_ = std::max(1, dragged.getHeight());
    external_gap_move_centre_y_ = 0;
    external_gap_move_direction_ = 0;
    insertion_region_->setColor(findColour(Skin::kFXAccent, true));
    insertion_region_->setRounding(std::max(0.5f, findValue(Skin::kBodyRounding)));
    insertion_region_->setAlpha(kInsertionPreviewAlpha);
    drag_boundary_bands_->setColor(findColour(Skin::kFXAccent, true));
    drag_boundary_bands_->setAlpha(kInsertionPreviewAlpha);
    for (auto& section : module_sections)
        section->setDragVisual(section.get() == dragged_module_ && external_source_excluded_
                                   ? ModuleSection::DragVisual::kDragged
                                   : ModuleSection::DragVisual::kDimmed);
    drop_target_module_ = nullptr;

    // Establish the stable entry gap from the hosted module's actual bounds in
    // this container's (independently scrolled) coordinate space, not pointer Y.
    const auto dragged_in_container = container_->getLocalArea(
        nullptr, dragged.getScreenBounds());
    external_target_insertion_index_ = calculateExternalInsertionIndex(dragged_in_container);
    setEffectPositions();
    redoBackgroundImage();
    return updateExternalTargetHover(dragged, pointerScreen);
}

int EffectModuleSection::updateExternalTargetHover(ModuleSection& dragged,
                                                   juce::Point<int> pointerScreen) {
    if (!external_target_preview_active_)
        return -1;

    external_target_gap_height_ = std::max(1, dragged.getHeight());
    const auto pointer_in_viewport = viewport_.getLocalPoint(nullptr, pointerScreen);
    viewport_.autoScroll(pointer_in_viewport.x, pointer_in_viewport.y, 40, 8);

    // Same shared reorder routine as vertical mode; only the displacement rule
    // differs while the ghost floats free (ghost/module overlap vs center crossing).
    const auto dragged_in_container = container_->getLocalArea(
        nullptr, dragged.getScreenBounds());
    updateVerticalReorderPreview(dragged, dragged_in_container, true, true);
    return external_target_insertion_index_;
}

int EffectModuleSection::beginExternalTargetVerticalDrag(
    ModuleSection& dragged, juce::Point<int> pointerScreen) {
    return updateExternalTargetVerticalDrag(dragged, pointerScreen);
}

int EffectModuleSection::updateExternalTargetVerticalDrag(
    ModuleSection& dragged, juce::Point<int> pointerScreen) {
    if (!external_target_preview_active_)
        return -1;

    const auto pointer_in_viewport = viewport_.getLocalPoint(nullptr, pointerScreen);
    viewport_.autoScroll(pointer_in_viewport.x, pointer_in_viewport.y, 40, 8);

    const auto dragged_in_container = container_->getLocalArea(
        nullptr, dragged.getScreenBounds());
    updateVerticalReorderPreview(dragged, dragged_in_container, true);
    return external_target_insertion_index_;
}

void EffectModuleSection::clearExternalTargetPreview() {
    if (!external_target_preview_active_)
        return;
    external_target_preview_active_ = false;
    external_target_insertion_index_ = -1;
    external_target_gap_height_ = 0;
    external_gap_move_centre_y_ = 0;
    external_gap_move_direction_ = 0;
    insertion_region_->setAlpha(0.0f);
    drag_boundary_bands_->setAlpha(0.0f);
    drag_boundary_bands_->setNumQuads(0);
    drop_target_module_ = nullptr;
    for (auto& section : module_sections)
        section->setDragVisual(section.get() == dragged_module_ && external_source_excluded_
                                   ? ModuleSection::DragVisual::kDragged
                                   : ModuleSection::DragVisual::kNormal);
    setEffectPositions();
    redoBackgroundImage();
}

void EffectModuleSection::clearDragSession() {
    for (auto& section : module_sections)
        section->setDragVisual(ModuleSection::DragVisual::kNormal);
    if (dragged_module_ != nullptr) {
        if (dragged_module_->getParentComponent() != container_.get())
            container_->addAndMakeVisible(dragged_module_);
        dragged_module_->setHorizontalDragOwnedExternally(false);
        dragged_module_->setExternallyVisualHosted(false);
        dragged_module_->setInterceptsMouseClicks(true, true);
        dragged_module_->setAlwaysOnTop(false);
    }
    container_->setBakeExcludedChild(nullptr);
    external_source_excluded_ = false;
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
