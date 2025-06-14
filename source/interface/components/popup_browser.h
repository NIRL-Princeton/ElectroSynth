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

#include "open_gl_image.h"
#include "open_gl_image_component.h"
#include "open_gl_multi_quad.h"
#include "synth_section.h"
#include "default_look_and_feel.h"
class PopupDisplay : public SynthSection {
public:
    PopupDisplay();

    void resized() override;

    void setContent(const std::string& text, juce::Rectangle<int> bounds, juce::BubbleComponent::BubblePlacement placement);

private:
    std::shared_ptr<PlainTextComponent> text_;
    std::shared_ptr<OpenGlQuad> body_;
    std::shared_ptr<OpenGlQuad> border_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupDisplay)
};


class OpenGlBorder : public OpenGlAutoImageComponent<juce::ResizableBorderComponent> {
public:
    OpenGlBorder(juce::Component* componentToResize, juce::ComponentBoundsConstrainer* constrainer) :
            OpenGlAutoImageComponent<juce::ResizableBorderComponent>(componentToResize, constrainer) {
        image_component_ = std::make_shared<OpenGlImageComponent>();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
    }

    virtual void resized() override {
        OpenGlAutoImageComponent<juce::ResizableBorderComponent>::resized();
        if (isShowing())
            redoImage();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlBorder)
};

class PopupList : public SynthSection, juce::ScrollBar::Listener {
public:
    class Listener {
    public:
        virtual ~Listener() = default;

        virtual void newSelection(PopupList* list, int id, int index) = 0;
        virtual void doubleClickedSelected(PopupList* list, int id, int index) { }
    };

    static constexpr float kRowHeight = 32.0f;
    static constexpr float kScrollSensitivity = 200.0f;
    static constexpr float kScrollBarWidth = 15.0f;

    PopupList();

    void paintBackground(juce::Graphics& g) override { }
    void paintBackgroundShadow(juce::Graphics& g) override { }
    void resized() override;

    void setSelections(PopupItems selections);
    PopupItems getSelectionItems(int index) const { return selections_.items[index]; }
    int getRowFromPosition(float mouse_position);
    int getRowHeight();
    int getTextPadding() { return getRowHeight() / 4; }
    int getBrowseWidth();
    int getBrowseHeight() { return getRowHeight() * selections_.size(); }
    juce::Font getFont() ;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    int getSelection(const juce::MouseEvent& e);

    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    void setSelected(int selection) { selected_ = selection; }
    int getSelected() const { return selected_; }
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void resetScrollPosition();
    void scrollBarMoved(juce::ScrollBar* scroll_bar, double range_start) override;
    void setScrollBarRange();
    int getScrollableRange();

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(juce::OpenGLContext& open_gl) override;
    void addListener(Listener* listener) {
        listeners_.push_back(listener);
    }
    void showSelected(bool show) { show_selected_ = show; }
    void select(int select);

private:
    int getViewPosition() {
        int view_height = getHeight();
        return std::max(0, std::min<int>(selections_.size() * getRowHeight() - view_height, view_position_));
    }
    void redoImage();
    void moveQuadToRow(OpenGlQuad& quad, int row);

    std::vector<Listener*> listeners_;
    PopupItems selections_;
    int selected_;
    int hovered_;
    bool show_selected_;

    float view_position_;
    std::unique_ptr<OpenGlScrollBar> scroll_bar_;
    std::shared_ptr<OpenGlImage> rows_;
    std::shared_ptr<OpenGlQuad> highlight_;
    std::shared_ptr<OpenGlQuad> hover_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupList)
};



class SinglePopupSelector : public SynthSection, public PopupList::Listener {
public:
    SinglePopupSelector();

    void paintBackground(juce::Graphics& g) override { }
    void paintBackgroundShadow(juce::Graphics& g) override { }
    void resized() override;

    void visibilityChanged() override {
        if (isShowing() && isVisible())
            grabKeyboardFocus();
    }

    void setPosition(juce::Point<int> position, juce::Rectangle<int> bounds);

    void newSelection(PopupList* list, int id, int index) override {
        if (id >= 0) {
            cancel_ = nullptr;
            callback_(id);
            setVisible(false);
        }
        else
            cancel_();
    }

    void focusLost(FocusChangeType cause) override {
        setVisible(false);
        if (cancel_)
            cancel_();
    }

    void setCallback(std::function<void(int)> callback) { callback_ = std::move(callback); }
    void setCancelCallback(std::function<void()> cancel) { cancel_ = std::move(cancel); }

    void showSelections(const PopupItems& selections) {
        popup_list_->setSelections(selections);
    }

private:
    std::shared_ptr<OpenGlQuad> body_;
    std::shared_ptr<OpenGlQuad> border_;

    std::function<void(int)> callback_;
    std::function<void()> cancel_;
    std::unique_ptr<PopupList> popup_list_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SinglePopupSelector)
};

class DualPopupSelector : public SynthSection, public PopupList::Listener {
public:
    DualPopupSelector();

    void paintBackground(juce::Graphics& g) override { }
    void paintBackgroundShadow(juce::Graphics& g) override { }
    void resized() override;
    void visibilityChanged() override {
        if (isShowing() && isVisible())
            grabKeyboardFocus();
    }

    void setPosition(juce::Point<int> position, int width, juce::Rectangle<int> bounds);

    void newSelection(PopupList* list, int id, int index) override;
    void doubleClickedSelected(PopupList* list, int id, int index) override { setVisible(false); }

    void focusLost(FocusChangeType cause) override { setVisible(false); }

    void setCallback(std::function<void(int)> callback) { callback_ = std::move(callback); }

    void showSelections(const PopupItems& selections) {
        left_list_->setSelections(selections);

        for (int i = 0; i < selections.size(); ++i) {
            if (selections.items[i].selected)
                right_list_->setSelections(selections.items[i]);
        }
    }

private:
    std::shared_ptr<OpenGlQuad> body_;
    std::shared_ptr<OpenGlQuad> border_;
    std::shared_ptr<OpenGlQuad> divider_;

    std::function<void(int)> callback_;
    std::unique_ptr<PopupList> left_list_;
    std::unique_ptr<PopupList> right_list_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DualPopupSelector)
};

class PopupClosingArea : public juce::Component {
public:
    PopupClosingArea() : juce::Component("Ignore Area") { }

    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void closingAreaClicked(PopupClosingArea* closing_area, const juce::MouseEvent& e) = 0;
    };

    void mouseDown(const juce::MouseEvent& e) override {
        for (Listener* listener : listeners_)
            listener->closingAreaClicked(this, e);
    }

    void addListener(Listener* listener) { listeners_.push_back(listener); }

private:
    std::vector<Listener*> listeners_;
};