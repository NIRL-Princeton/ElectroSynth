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
#include "skin.h"

ConnectionButton::ConnectionButton(String name) : EndpointArrowComponent(std::move(name)),
    initialized(false), mouse_state_(kNone) {

    setWantsKeyboardFocus(true);
    setComponentID("mod");
    setComponent(&drag_drop_area_);
    addAndMakeVisible(drag_drop_area_);
    drag_drop_area_.setInterceptsMouseClicks(false, false);
    source_color_ = findColour(Skin::kWidgetPrimary1, true);
    setArrowColor(source_color_);
}

ConnectionButton::~ConnectionButton() = default;

void ConnectionButton::resized() {
  EndpointArrowComponent::resized();
  drag_drop_area_.setBounds(getLocalBounds().reduced(4));
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
