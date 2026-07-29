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


#include "open_gl_image_component.h"

namespace electrosynth {
  struct Connection;
} // namespace vital

class SynthGuiInterface;

class ConnectionButton : public PlainShapeComponent {
  public:
    static constexpr float kFontAreaHeightRatio = 0.3f;
    static constexpr int kColumns = 3;
    static constexpr int kRows = 2;
    static constexpr int kMaxKnobs = kRows * kColumns;
    static constexpr float kMeterAreaRatio = 0.05f;

    enum MenuId {
      kCancel = 0,
      kDisconnect,
      kConnectionList
    };

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

        virtual void connectionChanged() { }
        virtual void connectionDisconnected(electrosynth::Connection* connection, bool last) { }
        virtual void connectionSelected(ConnectionButton* source) { }
        virtual void mappingLostFocus(ConnectionButton* source) { }
        virtual void startDestinationMap(ConnectionButton* source, const MouseEvent& e) { }
        virtual void mappingDragged(const MouseEvent& e) { }
        virtual void connectionWheelMoved(const MouseEvent& e, const MouseWheelDetails& wheel) { }
        virtual void endDestinationMap() { }
        virtual void connectionClicked(ConnectionButton* source) { }
        virtual void connectionCleared() { }
    };
  
    ConnectionButton(String name);
    virtual ~ConnectionButton();
    void init(OpenGlWrapper& ) override;
    void paintBackground(Graphics& g) override;
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
    void disconnectIndex(int index);

    void select(bool select);
    bool isSelected() const { return selected_; }
    void setActiveConnection(bool active);
    bool isActiveConnection() const { return active_connection_; }

    void setForceEnableModulationSource();
    bool hasAnyConnection();
    void setSourceColor(juce::Colour color);
    juce::Colour getSourceColor() const { return source_color_; }
    void setFontSize(float size) { font_size_ = size; }
    Rectangle<int> getModulationAmountBounds(int index, int total);
    Rectangle<int> getModulationAreaBounds();
    Rectangle<int> getMeterBounds();
    void setConnectRight(bool connect) { connect_right_ = connect; repaint(); }
    void setDrawBorder(bool border) { draw_border_ = border; repaint(); }
    void overrideText(String text) { text_override_ = std::move(text); repaint(); }
    void setDisplayLabel(juce::String label) { display_label_ = std::move(label); }
    juce::String getDisplayLabel() const { return display_label_; }
    juce::String display_label_;

  private:
    void disconnectConnection(electrosynth::Connection* connection);
    bool initialized;
    String text_override_;
    SynthGuiInterface* parent_;
    std::vector<Listener*> listeners_;
    MouseState mouse_state_;
    bool selected_;
    bool connect_right_;
    bool draw_border_;
    bool active_connection_;
    OpenGlImageComponent drag_drop_;
    Component drag_drop_area_;
    float font_size_;

    Colour drag_drop_color_;
    Colour source_color_;
    Colour background_color_;
    bool show_drag_drop_;
    float drag_drop_alpha_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectionButton)
};
