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

#include "juce_gui_basics/juce_gui_basics.h"

class SynthSlider;

class LeftAlignedScrollBar : public juce::ScrollBar {
  public:
    LeftAlignedScrollBar(bool vertical) : ScrollBar(vertical) { }
};

class DefaultLookAndFeel : public juce::LookAndFeel_V4 {
  public:
    static constexpr int kPopupMenuBorder = 8;

    ~DefaultLookAndFeel() { }

    virtual int getPopupMenuBorderSize() override { return kPopupMenuBorder; }

    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& text_editor) override { }
    void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& text_editor) override;
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(juce::ComboBox& box, juce::Label& label) override;

    virtual void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scroll_bar, int x, int y, int width, int height,
                               bool vertical, int thumb_position, int thumb_size,
                               bool mouse_over, bool mouse_down) override;

    void drawComboBox(juce::Graphics& g, int width, int height, const bool button_down,
                      int button_x, int button_y, int button_w, int button_h, juce::ComboBox& box) override;

    void drawTickBox(juce::Graphics& g, juce::Component& component,
                     float x, float y, float w, float h, bool ticked,
                     bool enabled, bool mouse_over, bool button_down) override;

    void drawCallOutBoxBackground(juce::CallOutBox& call_out_box, juce::Graphics& g, const juce::Path& path, juce::Image&) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& background_color,
                              bool hover, bool down) override;

//    int getSliderPopupPlacement(juce::Slider& slider) override;

    juce::Font getPopupMenuFont() override;
    juce::Font getSliderPopupFont(juce::Slider& slider) override;
  
    int getMenuWindowFlags() override { return 0; }

    static DefaultLookAndFeel* instance() {
      static DefaultLookAndFeel instance;
      return &instance;
    }

  protected:
    DefaultLookAndFeel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DefaultLookAndFeel)
};
