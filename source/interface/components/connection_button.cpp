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

#include "connection_button.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "paths.h"
#include "skin.h"
#include "synth_base.h"
#include "synth_gui_interface.h"
//#include "modulation_matrix.h"
#include "synth_section.h"

ConnectionButton:: ConnectionButton(String name) : PlainShapeComponent(std::move(name)), parent_(nullptr),
                                                  mouse_state_(kNone), selected_(false), connect_right_(false),
                                                  draw_border_(false), active_connection_(false), font_size_(12.0f),
                                                  drag_drop_color_(juce::Colours::white),
                                                  source_color_(juce::Colours::white),
                                                  background_color_(juce::Colours::black),
                                                  show_drag_drop_(true), drag_drop_alpha_(1.0f), initialized(false) {
  setWantsKeyboardFocus(true);
  setComponentID("mod");
  Path shape = Paths::dragDropArrows();
  //shape.addLineSegment(Line<float>(-50.0f, -50.0f, -50.0f, -50.0f), 0.2f);
  setShape(Paths::dragDropArrows());
  setComponent(&drag_drop_area_);
  setActive(true);
  setUseAlpha(true);
  setInterceptsMouseClicks(true, false);
  addAndMakeVisible(drag_drop_area_);
  drag_drop_area_.setInterceptsMouseClicks(false, false);
  setColor(source_color_);
}

ConnectionButton::~ConnectionButton() {
//  if (parent_)
//    parent_->getSynth()->forceShowModulation(getName().toStdString(), false);
}

bool ConnectionButton::hasAnyConnection() {
  if (parent_)
//    return parent_->getSynth()->isSourceConnected(getName().toStdString());
  return false;
}

Rectangle<int> ConnectionButton::getModulationAmountBounds(int index, int total) {
  int columns = kColumns;

  int row = index / columns;
  int column = index % columns;

  Rectangle<int> all_bounds = getModulationAreaBounds();
  int x = all_bounds.getX() + (all_bounds.getWidth() * column) / columns;
  int right = all_bounds.getX() + (all_bounds.getWidth() * (column + 1)) / columns;
  int width = right - x;
  int y = all_bounds.getY() + all_bounds.getHeight() - width * (row + 1);
  return Rectangle<int>(x, y, width, width);
}

Rectangle<int> ConnectionButton::getMeterBounds() {
  static constexpr int kMinMeterWidth = 4;

  int width = getWidth();
  int meter_width = std::max<int>(kMinMeterWidth, std::round(width * kMeterAreaRatio / 2.0f) * 2);
  int meter_height = getHeight() - 2;
  return Rectangle<int>(1, 1, meter_width, meter_height);
}

Rectangle<int> ConnectionButton::getModulationAreaBounds() {
  static constexpr int kMaxWidthHeightRatio = 3;

  SynthSection* parent = findParentComponentOfClass<SynthSection>();
  int widget_margin = 0;
  if (parent)
    widget_margin = parent->findValue(Skin::kWidgetMargin);

  int width = getWidth() - getMeterBounds().getRight();
  int height = getHeight();

  int widget_width = width - 2 * widget_margin;
  int knob_width = widget_width / kColumns;
  widget_width = knob_width * kColumns;
  int widget_x = getMeterBounds().getRight() + (width - widget_width) / 2;
  int min_y = kFontAreaHeightRatio * width;
  int max_widget_height = ceilf(widget_width * 2.0f / 3.0f);
  int widget_y = std::max(min_y, height - widget_margin - max_widget_height);
  int widget_height = height - widget_y - widget_margin;
  int center_y = widget_y + widget_height / 2;
  widget_height = std::max(widget_height, (widget_width + kMaxWidthHeightRatio - 1) / kMaxWidthHeightRatio);
  widget_y = center_y - widget_height / 2;
  return Rectangle<int>(widget_x, widget_y, widget_width, widget_height);
}

void ConnectionButton::paintBackground(Graphics& g) {
  if (getWidth() == 0 || getHeight() == 0)
    return;

  SynthSection* parent = findParentComponentOfClass<SynthSection>();
  int rounding_amount = 4;
  if (parent)
    rounding_amount = parent->findValue(Skin::kBodyRounding);

  const auto bounds = getLocalBounds().toFloat();
  g.setColour(background_color_);
  g.fillRoundedRectangle(bounds, rounding_amount);
  g.setColour(source_color_);
  g.drawRoundedRectangle(bounds.reduced(0.5f), rounding_amount, selected_ ? 2.0f : 1.0f);
}

void ConnectionButton::parentHierarchyChanged() {
  if (parent_ == nullptr) {
    parent_ = findParentComponentOfClass<SynthGuiInterface>();
    setForceEnableModulationSource();
  }
}

void ConnectionButton::resized() {
  PlainShapeComponent::resized();
  drag_drop_area_.setBounds(getLocalBounds().reduced(4));
}

void ConnectionButton::render(OpenGlWrapper& open_gl, bool animate) {
  static constexpr float kDeltaAlpha = 0.15f;

  float target = 1.0f;
  if (mouse_state_ == kMouseDown || mouse_state_ == kMouseDragging || mouse_state_ == kDraggingOut)
    target = 1.35f;

  bool increase = drag_drop_alpha_ < target;
  if (increase)
    drag_drop_alpha_ = std::min(drag_drop_alpha_ + kDeltaAlpha, target);
  else
    drag_drop_alpha_ = std::max(drag_drop_alpha_ - kDeltaAlpha, target);

  if (drag_drop_alpha_ <= 0.0f) {
    drag_drop_alpha_ = 0.0f;
    setActive(false);
  }

  setColor(drag_drop_color_.withMultipliedAlpha(drag_drop_alpha_));
  PlainShapeComponent::render(open_gl, animate);
}

void ConnectionButton::init(OpenGlWrapper &open_gl) {
    PlainShapeComponent::init(open_gl);
    //DBG(juce::String(image_.shader()->getProgramID()));
    if (image_.shader()->getProgramID() !=  0)
        initialized = true;
}

bool ConnectionButton::isInit() {
    return initialized;
}

void ConnectionButton::mouseDown(const MouseEvent& e) {
    DBG(getComponentID() + "currmode");
  if (e.mods.isPopupMenu()) {
    if (parent_ == nullptr)
      return;

//    std::vector<vital::ModulationConnection*> connections =
//        parent_->getSynth()->getSourceConnections(getName().toStdString());

//    if (connections.empty())
//      return;

    mouse_state_ = kNone;

//    PopupItems options;
//    std::string disconnect = "Disconnect from ";
//    for (int i = 0; i < connections.size(); ++i) {
//      std::string destination = vital::Parameters::getDisplayName(connections[i]->destination_name);
//      options.addItem(kModulationList + i, disconnect + destination);
//    }
//
//    if (connections.size() > 1)
//      options.addItem(kDisconnect, "Disconnect all");
//
//    SynthSection* parent = findParentComponentOfClass<SynthSection>();
//    parent->showPopupSelector(this, e.getPosition(), options, [=](int selection) { disconnectIndex(selection); });
  }
  else {
    setActiveConnection(true);
    mouse_state_ = kMouseDown;

    for (Listener* listener : listeners_)
      listener->connectionSelected(this);
  }
}

void ConnectionButton::mouseDrag(const MouseEvent& e) {
  if (e.mods.isRightButtonDown())
    return;

  if (!getLocalBounds().contains(e.getPosition()) && mouse_state_ != kDraggingOut) {
    for (Listener* listener : listeners_)
      listener->startDestinationMap(this, e);
    mouse_state_ = kDraggingOut;
    setMouseCursor(MouseCursor::DraggingHandCursor);
  }

  if (mouse_state_ == kDraggingOut) {
    for (Listener* listener : listeners_)
      listener->mappingDragged(e);
  }
  else if (mouse_state_ != kMouseDragging)
    mouse_state_ = kMouseDragging;
}

void ConnectionButton::mouseUp(const MouseEvent& e) {
  if (!e.mods.isRightButtonDown() && mouse_state_ == kDraggingOut) {
    for (Listener* listener : listeners_)
      listener->endDestinationMap();
  }
  else if (!e.mods.isRightButtonDown()) {
    for (Listener* listener : listeners_)
      listener->connectionClicked(this);
  }
  setMouseCursor(MouseCursor::ParentCursor);

  mouse_state_ = kHover;
}

void ConnectionButton::mouseEnter(const MouseEvent& e) {
  mouse_state_ = kHover;
  drag_drop_color_ = source_color_;
  show_drag_drop_ = true;//parent_->getSynth()->getSourceConnections(getName().toStdString()).empty();
  setActive(show_drag_drop_);
  redrawImage(true);
}

void ConnectionButton::mouseExit(const MouseEvent& e) {
  mouse_state_ = kNone;
  show_drag_drop_ = true;
}

void ConnectionButton::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  for (Listener* listener : listeners_)
    listener->connectionWheelMoved(e, wheel);
}

void ConnectionButton::focusLost(FocusChangeType cause) {
  for (Listener* listener : listeners_)
    listener->mappingLostFocus(this);
}

void ConnectionButton::addListener(Listener* listener) {
  listeners_.push_back(listener);
}

void ConnectionButton::setSourceColor(juce::Colour color) {
  source_color_ = color;
  drag_drop_color_ = color;
  setColor(source_color_.withMultipliedAlpha(drag_drop_alpha_));
  repaintBackground();
  redrawImage(true);
}

void ConnectionButton::disconnectIndex(int index) {
  if (parent_ == nullptr)
    return;

//  std::vector<vital::ModulationConnection*> connections =
//      parent_->getSynth()->getSourceConnections(getName().toStdString());

//  if (index == kDisconnect) {
//    for (vital::ModulationConnection* connection : connections)
//      disconnectModulation(connection);
//  }
//  else if (index >= kModulationList) {
//    int connection_index = index - kModulationList;
//    disconnectModulation(connections[connection_index]);
//  }
}

void ConnectionButton::select(bool select) {
  selected_ = select;
  setForceEnableModulationSource();
}

void ConnectionButton::setActiveConnection(bool active) {
  active_connection_ = active;
  setForceEnableModulationSource();
}

void ConnectionButton::setForceEnableModulationSource() {
//  if (parent_)
//    parent_->getSynth()->forceShowModulation(getName().toStdString(), active_modulation_);
}

void ConnectionButton::disconnectConnection(electrosynth::Connection* connection) {
////  int modulations_left = parent_->getSynth()->getNumModulations(connection->destination_name);
//
//  for (Listener* listener : listeners_) {
//    listener->modulationDisconnected(connection, modulations_left <= 1);
//    listener->modulationConnectionChanged();
//  }
//
////  parent_->disconnectModulation(connection);
//
//  if (modulations_left <= 1) {
//    for (Listener* listener : listeners_)
//      listener->modulationCleared();
//  }
}
