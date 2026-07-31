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
#include "synth_section.h"

ConnectionButton::ConnectionButton(String name)
    : EndpointArrowComponent(std::move(name)), initialized(false), parent_(nullptr),
      mouse_state_(kNone), active_connection_(false) {
  setWantsKeyboardFocus(true);
  setComponentID("mod");
  setComponent(&drag_drop_area_);
  addAndMakeVisible(drag_drop_area_);
  drag_drop_area_.setInterceptsMouseClicks(false, false);
  source_color_ = findColour(Skin::kWidgetPrimary1, true);
  setArrowColor(source_color_);
}

ConnectionButton::~ConnectionButton() = default;

bool ConnectionButton::hasAnyConnection() {
  return parent_ != nullptr
      && !parent_->getSynth()->getSourceConnections(getComponentID().toStdString()).empty();
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

void ConnectionButton::parentHierarchyChanged() {
  if (parent_ == nullptr)
    parent_ = findParentComponentOfClass<SynthGuiInterface>();
}

void ConnectionButton::resized() {
  PlainShapeComponent::resized();
  drag_drop_area_.setBounds(getLocalBounds().reduced(4));
}

void ConnectionButton::render(OpenGlWrapper& open_gl, bool animate) {
  EndpointArrowComponent::render(open_gl, animate);
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
  if (e.mods.isPopupMenu())
    return;

  setActiveConnection(true);
  mouse_state_ = kMouseDown;

  for (Listener* listener : listeners_)
    listener->connectionSelected(this);
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
  EndpointArrowComponent::mouseEnter(e);
  mouse_state_ = kHover;
  setActive(true);
  redrawImage(true);
}

void ConnectionButton::mouseExit(const MouseEvent& e) {
  EndpointArrowComponent::mouseExit(e);
  mouse_state_ = kNone;
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
  if (listener != nullptr && std::find(listeners_.begin(), listeners_.end(), listener) == listeners_.end())
    listeners_.push_back(listener);
}

void ConnectionButton::removeListener(Listener* listener) {
  std::erase(listeners_, listener);
}

void ConnectionButton::setSourceColor(juce::Colour color) {
  source_color_ = color;
  setArrowColor(color);
  repaintBackground();
  redrawImage(true);
}

void ConnectionButton::setActiveConnection(bool active) {
  active_connection_ = active;
}
