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

#pragma once


#include "endpoint_arrow_component.h"

class ConnectionButton : public EndpointArrowComponent {
  public:
    enum MouseState {
      kNone,
      kHover,
      kMouseDown,
      kMouseDragging,
      kDraggingOut
    };

    class Listener {
      public:
        virtual ~Listener() = default;

        virtual void connectionSelected(ConnectionButton* source) { }
        virtual void mappingLostFocus(ConnectionButton* source) { }
        virtual void startDestinationMap(ConnectionButton* source, const MouseEvent& e) { }
        virtual void mappingDragged(const MouseEvent& e) { }
        virtual void connectionWheelMoved(const MouseEvent& e, const MouseWheelDetails& wheel) { }
        virtual void endDestinationMap() { }
        virtual void connectionClicked(ConnectionButton* source) { }
    };

    explicit ConnectionButton(
        String name,
        electrosynth::EndpointDescriptor endpoint = {});
    virtual ~ConnectionButton();
    void init(OpenGlWrapper& ) override;
    void resized() override;
    bool isInit() override;

    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseEnter(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    void focusLost(FocusChangeType cause) override;
    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    void setSourceColor(juce::Colour color);
    juce::Colour getSourceColor() const { return source_color_; }

    void setDisplayLabel(juce::String label) { display_label_ = std::move(label); }
    juce::String getDisplayLabel() const { return display_label_; }
    juce::String display_label_;

  private:
    bool initialized;
    std::vector<Listener*> listeners_;
    MouseState mouse_state_;

    Component drag_drop_area_;

    Colour source_color_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectionButton)
};
