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

class SynthGuiInterface;

class ConnectionButton : public EndpointArrowComponent {
  public:
    static constexpr float kFontAreaHeightRatio = 0.3f;
    static constexpr int kColumns = 3;
    static constexpr int kRows = 2;
    static constexpr int kMaxKnobs = kRows * kColumns;
    static constexpr float kMeterAreaRatio = 0.05f;

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
  
    ConnectionButton(String name);
    virtual ~ConnectionButton();
    void init(OpenGlWrapper& ) override;
    void parentHierarchyChanged() override;
    void resized() override;
    bool isInit() override;
    virtual void render(OpenGlWrapper& open_gl, bool animate) override;

    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseEnter(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    void focusLost(FocusChangeType cause) override;
    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    void setActiveConnection(bool active);
    bool isActiveConnection() const { return active_connection_; }

    bool hasAnyConnection();
    void setSourceColor(juce::Colour color);
    juce::Colour getSourceColor() const { return source_color_; }
    Rectangle<int> getModulationAmountBounds(int index, int total);
    Rectangle<int> getModulationAreaBounds();
    Rectangle<int> getMeterBounds();
    void setDisplayLabel(juce::String label) { display_label_ = std::move(label); }
    juce::String getDisplayLabel() const { return display_label_; }
    juce::String display_label_;

  private:
    bool initialized;
    SynthGuiInterface* parent_;
    std::vector<Listener*> listeners_;
    MouseState mouse_state_;
    bool active_connection_;
    Component drag_drop_area_;

    Colour source_color_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectionButton)
};
