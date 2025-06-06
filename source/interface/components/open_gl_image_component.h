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

#include "open_gl_component.h"
#include "look_and_feel/fonts.h"
#include "open_gl_image.h"

class SynthSection;

class OpenGlImageComponent : public OpenGlComponent {
public:
    OpenGlImageComponent(juce::String name = "");
    virtual ~OpenGlImageComponent() = default;

    virtual void paintBackground(juce::Graphics& g) override {
        redrawImage(false, true);
    }

    virtual void paintToImage(juce::Graphics& g) {
        juce::Component* component = component_ ? component_ : this;
        if (paint_entire_component_)
            component->paintEntireComponent(g, false);
        else
            component->paint(g);
    }

    virtual void init(OpenGlWrapper& open_gl) override;
    virtual void render(OpenGlWrapper& open_gl, bool animate) override;
    virtual void destroy(juce::OpenGLContext& open_gl) override;
    virtual bool isInit() override;
    virtual void redrawImage(bool force, bool clear=true);
    void setComponent(juce::Component* component) { component_ = component; }
    void setScissor(bool scissor) { image_.setScissor(scissor); }
    void setUseAlpha(bool use_alpha) { image_.setUseAlpha(use_alpha); }
    void setColor(juce::Colour color) { image_.setColor(color); }
    OpenGlImage& image() { return image_; }
    void setActive(bool active) { active_ = active; }
    void setStatic(bool static_image) { static_image_ = static_image; }
    void paintEntireComponent(bool paint_entire_component) { paint_entire_component_ = paint_entire_component; }
    bool isActive() const { return active_; }
    std::unique_ptr<juce::Image> draw_image_;
protected:
    juce::Component* component_;
    bool active_;
    bool static_image_;
    bool paint_entire_component_;

    OpenGlImage image_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlImageComponent)
};


template <class ComponentType>
class OpenGlAutoImageComponent : public ComponentType {
  public:
    using ComponentType::ComponentType;

    virtual void mouseDown(const juce::MouseEvent& e) override {
      ComponentType::mouseDown(e);
      redoImage();
    }

    virtual void mouseUp(const juce::MouseEvent& e) override {
      ComponentType::mouseUp(e);
      redoImage();
    }

    virtual void mouseDoubleClick(const juce::MouseEvent& e) override {
      ComponentType::mouseDoubleClick(e);
      redoImage();
    }

    virtual void mouseEnter(const juce::MouseEvent& e) override {
      ComponentType::mouseEnter(e);
      redoImage();
    }

    virtual void mouseExit(const juce::MouseEvent& e) override {
      ComponentType::mouseExit(e);
      redoImage();
    }

    virtual void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override {
      ComponentType::mouseWheelMove(e, wheel);
      redoImage();
    }

    std::shared_ptr<OpenGlImageComponent> getImageComponent() { return image_component_; }
    virtual void redoImage() { image_component_->redrawImage(true); }

  protected:
    std::shared_ptr<OpenGlImageComponent> image_component_;
};

class OpenGlTextEditor : public OpenGlAutoImageComponent<juce::TextEditor>, public juce::TextEditor::Listener {
  public:
    OpenGlTextEditor(juce::String name) : OpenGlAutoImageComponent(name) {
        image_component_ = std::make_shared<OpenGlImageComponent>(name + "_text");
      monospace_ = false;
      image_component_->setComponent(this);
      addListener(this);
    }
  
    OpenGlTextEditor(juce::String name, wchar_t password_char) : OpenGlAutoImageComponent(name, password_char) {
      monospace_ = false;
        image_component_ = std::make_shared<OpenGlImageComponent>();
      image_component_->setComponent(this);
      addListener(this);
    }

    bool keyPressed(const juce::KeyPress& key) override {
      bool result = juce::TextEditor::keyPressed(key);
      redoImage();
      return result;
    }
  
    void textEditorTextChanged(juce::TextEditor&) override { redoImage(); }
    void textEditorFocusLost(juce::TextEditor&) override { redoImage(); }

    virtual void mouseDrag(const juce::MouseEvent& e) override {
      juce::TextEditor::mouseDrag(e);
      redoImage();
    }

    void applyFont() {
      juce::Font font;
      if (monospace_)
        font = Fonts::instance()->monospace().withPointHeight(getHeight() / 2.0f);
      else
        font = Fonts::instance()->proportional_light().withPointHeight(getHeight() / 2.0f);

      applyFontToAllText(font);
      redoImage();
    }

    void visibilityChanged() override {
      juce::TextEditor::visibilityChanged();

      if (isVisible() && !isMultiLine())
        applyFont();
    }

    void resized() override {
      juce::TextEditor::resized();
      if (isMultiLine()) {
        float indent = image_component_->findValue(Skin::kLabelBackgroundRounding);
        setIndents(indent, indent);
        return;
      }

      if (monospace_)
        setIndents(getHeight() * 0.2, getHeight() * 0.17);
      else
        setIndents(getHeight() * 0.2, getHeight() * 0.15);
      if (isVisible())
        applyFont();
    }

    void setMonospace() {
      monospace_ = true;
    }

  private:
    bool monospace_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlTextEditor)
};

class PlainTextComponent : public OpenGlImageComponent {
  public:
    enum FontType {
      kTitle,
      kLight,
      kRegular,
      kMono,
      kNumFontTypes
    };

    PlainTextComponent(juce::String name, juce::String text) : OpenGlImageComponent(name), text_(std::move(text)),
                                                   text_size_(1.0f), font_type_(kRegular),
                                                   justification_(juce::Justification::centred),
                                                   buffer_(0) {
      setInterceptsMouseClicks(false, false);
    }

    void resized() override {
      OpenGlImageComponent::resized();
      redrawImage(true);
    }

    void setText(juce::String text) {
      if (text_ == text)
        return;

      text_ = text;
      redrawImage(true);
    }

    juce::String getText() const { return text_; }

    void paintToImage(juce::Graphics& g) override {
      g.setColour(juce::Colours::white);

      if (font_type_ == kTitle)
        g.setFont(Fonts::instance()->proportional_title().withPointHeight(text_size_));
      else if (font_type_ == kLight)
        g.setFont(Fonts::instance()->proportional_light().withPointHeight(text_size_));
      else if (font_type_ == kRegular)
        g.setFont(Fonts::instance()->proportional_regular().withPointHeight(text_size_));
      else
        g.setFont(Fonts::instance()->monospace().withPointHeight(text_size_));

      juce::Component* component = component_ ? component_ : this;

      g.drawFittedText(text_, buffer_, 0, component->getWidth() - 2 * buffer_,
                       component->getHeight(), justification_, false);
    }

    void setTextSize(float size) {
      text_size_ = size;
      redrawImage(true);
    }

    void setFontType(FontType font_type) {
      font_type_ = font_type;
    }

    void setJustification(juce::Justification justification) {
      justification_ = justification;
    }

    void setBuffer(int buffer) { buffer_ = buffer; }

  private:
    juce::String text_;
    float text_size_;
    FontType font_type_;
    juce::Justification justification_;
    int buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlainTextComponent)
};

class PlainShapeComponent : public OpenGlImageComponent {
  public:
    PlainShapeComponent(juce::String name) : OpenGlImageComponent(name), justification_(juce::Justification::centred) {
      setInterceptsMouseClicks(false, false);
    }

    void paintToImage(juce::Graphics& g) override {
      juce::Component* component = component_ ? component_ : this;
      juce::Rectangle<float> bounds = component->getLocalBounds().toFloat();
      juce::Path shape = shape_;
      shape.applyTransform(shape.getTransformToScaleToFit(bounds, true, justification_));

      g.setColour(juce::Colours::white);
      g.fillPath(shape);
    }

    void setShape(juce::Path shape) {
      shape_ = shape;
      redrawImage(true);
    }

    void setJustification(juce::Justification justification) { justification_ = justification; }

  private:
    juce::Path shape_;
    juce::Justification justification_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlainShapeComponent)
};
