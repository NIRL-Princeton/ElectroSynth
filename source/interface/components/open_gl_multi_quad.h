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

#include <mutex>

class OpenGlMultiQuad : public OpenGlComponent {
public:
    static constexpr int kNumVertices = 4;
    static constexpr int kNumFloatsPerVertex = 10;
    static constexpr int kNumFloatsPerQuad = kNumVertices * kNumFloatsPerVertex;
    static constexpr int kNumIndicesPerQuad = 6;
    static constexpr float kThicknessDecay = 0.4f;
    static constexpr float kAlphaInc = 0.2f;

    OpenGlMultiQuad(int max_quads, Shaders::FragmentShader shader = Shaders::kColorFragment, juce::String name = "");

    virtual ~OpenGlMultiQuad();

    virtual void init(OpenGlWrapper &open_gl) override;

    virtual void render(OpenGlWrapper &open_gl, bool animate) override;

    virtual void destroy(juce::OpenGLContext& open_gl) override;

    virtual bool isInit() override;

    void paintBackground(juce::Graphics &g) override {
    }

    void resized() override {
        OpenGlComponent::resized();
        dirty_ = true;
    }

    void markDirty() {
        dirty_ = true;
    }

    void setFragmentShader(Shaders::FragmentShader shader) { fragment_shader_ = shader; }

    void setNumQuads(int num_quads) {
        _ASSERT(num_quads <= max_quads_);
        num_quads_ = num_quads;
        dirty_ = true;
    }

    force_inline void setColor(juce::Colour color) {
        color_ = color;
    }

    force_inline juce::Colour getColor() {
        return color_;
    }

    force_inline void setAltColor(juce::Colour color) {
        alt_color_ = color;
    }

    force_inline void setModColor(juce::Colour color) {
        mod_color_ = color;
    }

    force_inline void setThumbColor(juce::Colour color) {
        thumb_color_ = color;
    }

    force_inline void setThumbAmount(float amount) {
        thumb_amount_ = amount;
    }

    force_inline void setStartPos(float pos) {
        start_pos_ = pos;
    }

    force_inline void setMaxArc(float max_arc) {
        max_arc_ = max_arc;
    }

    force_inline float getMaxArc() {
        return max_arc_;
    }

    force_inline float getQuadX(int i) const {
        int index = kNumFloatsPerQuad * i;
        return data_[index];
    }

    force_inline float getQuadY(int i) const {
        int index = kNumFloatsPerQuad * i;
        return data_[index + 1];
    }

    force_inline float getQuadWidth(int i) const {
        int index = kNumFloatsPerQuad * i;
        return data_[2 * kNumFloatsPerVertex + index] - data_[index];
    }

    force_inline float getQuadHeight(int i) const {
        int index = kNumFloatsPerQuad * i;
        return data_[kNumFloatsPerVertex + index + 1] - data_[index + 1];
    }

    float *getVerticesData(int i) {
        int index = kNumFloatsPerQuad * i;
        return data_.get() + index;
    }

    void setRotatedCoordinates(int i, float x, float y, float w, float h) {
        _ASSERT(i < max_quads_);
        int index = i * kNumFloatsPerQuad;

        data_[index + 4] = x;
        data_[index + 5] = y + h;
        data_[kNumFloatsPerVertex + index + 4] = x + w;
        data_[kNumFloatsPerVertex + index + 5] = y + h;
        data_[2 * kNumFloatsPerVertex + index + 4] = x + w;
        data_[2 * kNumFloatsPerVertex + index + 5] = y;
        data_[3 * kNumFloatsPerVertex + index + 4] = x;
        data_[3 * kNumFloatsPerVertex + index + 5] = y;
    }

    void setCoordinates(int i, float x, float y, float w, float h) {
        _ASSERT(i < max_quads_);
        int index = i * kNumFloatsPerQuad;

        data_[index + 4] = x;
        data_[index + 5] = y;
        data_[kNumFloatsPerVertex + index + 4] = x;
        data_[kNumFloatsPerVertex + index + 5] = y + h;
        data_[2 * kNumFloatsPerVertex + index + 4] = x + w;
        data_[2 * kNumFloatsPerVertex + index + 5] = y + h;
        data_[3 * kNumFloatsPerVertex + index + 4] = x + w;
        data_[3 * kNumFloatsPerVertex + index + 5] = y;
    }

    void setShaderValue(int i, float shader_value, int value_index = 0) {
        _ASSERT(i < max_quads_);
        int index = i * kNumFloatsPerQuad + 6 + value_index;
        data_[index] = shader_value;
        data_[kNumFloatsPerVertex + index] = shader_value;
        data_[2 * kNumFloatsPerVertex + index] = shader_value;
        data_[3 * kNumFloatsPerVertex + index] = shader_value;
        dirty_ = true;
    }

    void setDimensions(int i, float quad_width, float quad_height, float full_width, float full_height) {
        int index = i * kNumFloatsPerQuad;
        float w = quad_width * full_width / 2.0f;
        float h = quad_height * full_height / 2.0f;

        data_[index + 2] = w;
        data_[index + 3] = h;
        data_[kNumFloatsPerVertex + index + 2] = w;
        data_[kNumFloatsPerVertex + index + 3] = h;
        data_[2 * kNumFloatsPerVertex + index + 2] = w;
        data_[2 * kNumFloatsPerVertex + index + 3] = h;
        data_[3 * kNumFloatsPerVertex + index + 2] = w;
        data_[3 * kNumFloatsPerVertex + index + 3] = h;
    }

    void setQuadHorizontal(int i, float x, float w) {
        _ASSERT(i < max_quads_);
        int index = i * kNumFloatsPerQuad;
        data_[index] = x;
        data_[kNumFloatsPerVertex + index] = x;
        data_[2 * kNumFloatsPerVertex + index] = x + w;
        data_[3 * kNumFloatsPerVertex + index] = x + w;

        dirty_ = true;
    }

    void setQuadVertical(int i, float y, float h) {
        _ASSERT(i < max_quads_);
        int index = i * kNumFloatsPerQuad;
        data_[index + 1] = y;
        data_[kNumFloatsPerVertex + index + 1] = y + h;
        data_[2 * kNumFloatsPerVertex + index + 1] = y + h;
        data_[3 * kNumFloatsPerVertex + index + 1] = y;

        dirty_ = true;
    }

    void setQuad(int i, float x, float y, float w, float h) {
        _ASSERT(i < max_quads_);
        int index = i * kNumFloatsPerQuad;
        data_[index] = x;
        data_[index + 1] = y;
        data_[kNumFloatsPerVertex + index] = x;
        data_[kNumFloatsPerVertex + index + 1] = y + h;
        data_[2 * kNumFloatsPerVertex + index] = x + w;
        data_[2 * kNumFloatsPerVertex + index + 1] = y + h;
        data_[3 * kNumFloatsPerVertex + index] = x + w;
        data_[3 * kNumFloatsPerVertex + index + 1] = y;

        dirty_ = true;
    }

    void setActive(bool active) {
        active_ = active;
    }

    void setThickness(float thickness, bool reset = false) {
        thickness_ = thickness;
        if (reset)
            current_thickness_ = thickness_;
    }

    void setRounding(float rounding) {
        float adjusted = 2.0f * rounding;
        if (adjusted != rounding_) {
            dirty_ = true;
            rounding_ = adjusted;
        }
    }

    void setTargetComponent(juce::Component *target_component) {
        target_component_ = target_component;
    }

    void setScissorComponent(juce::Component *scissor_component) {
        scissor_component_ = scissor_component;
    }

    juce::OpenGLShaderProgram *shader() { return shader_; }

    void setAdditive(bool additive) { additive_blending_ = additive; }

    void setAlpha(float alpha, bool reset = false) {
        alpha_mult_ = alpha;
        if (reset)
            current_alpha_mult_ = alpha;
    }

    void setDrawWhenNotVisible(bool draw) { draw_when_not_visible_ = draw; }

protected:
    juce::Component *target_component_;
    juce::Component *scissor_component_;
    Shaders::FragmentShader fragment_shader_;
    int max_quads_;
    int num_quads_;

    bool draw_when_not_visible_;
    bool active_;
    bool dirty_;
    juce::Colour color_;
    juce::Colour alt_color_;
    juce::Colour mod_color_;
    juce::Colour thumb_color_;
    float max_arc_;
    float thumb_amount_;
    float start_pos_;
    float current_alpha_mult_;
    float alpha_mult_;
    bool additive_blending_;
    float current_thickness_;
    float thickness_;
    float rounding_;

    std::unique_ptr<float[]> data_;
    std::unique_ptr<int[]> indices_;

    juce::OpenGLShaderProgram *shader_;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> color_uniform_;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> alt_color_uniform_;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> mod_color_uniform_;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> background_color_uniform_;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> thumb_color_uniform_;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> thickness_uniform_;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> rounding_uniform_;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> max_arc_uniform_;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> thumb_amount_uniform_;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> start_pos_uniform_;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> alpha_mult_uniform_;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> position_;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> dimensions_;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> coordinates_;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> shader_values_;

    GLuint vertex_buffer_;
    GLuint indices_buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlMultiQuad)
};

class OpenGlQuad : public OpenGlMultiQuad {
public:
    OpenGlQuad(Shaders::FragmentShader shader, juce::String name) : OpenGlMultiQuad(1, shader, name) {
        setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
    }

    explicit OpenGlQuad(Shaders::FragmentShader shader) : OpenGlMultiQuad(1, shader) {
        setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
    }
};

class OpenGlScrollQuad : public OpenGlQuad {
public:
    OpenGlScrollQuad() : OpenGlQuad(Shaders::kRoundedRectangleFragment), scroll_bar_(nullptr),
                         hover_(false), shrink_left_(false), hover_amount_(-1.0f) {
    }

    virtual void render(OpenGlWrapper &open_gl, bool animate) override ;

    void setHover(bool hover) { hover_ = hover; }
    void setShrinkLeft(bool shrink_left) { shrink_left_ = shrink_left; }
    void setScrollBar(juce::ScrollBar *scroll_bar) { scroll_bar_ = scroll_bar; }

private:
    juce::ScrollBar *scroll_bar_;
    bool hover_;
    bool shrink_left_;
    float hover_amount_;
};

class OpenGlScrollBar : public juce::ScrollBar {
public:
    OpenGlScrollBar(bool isVertical = true) : juce::ScrollBar(isVertical), bar_(new OpenGlScrollQuad()) {
        bar_->setTargetComponent(this);
        addAndMakeVisible(bar_.get());
        bar_->setScrollBar(this);
    }

    std::shared_ptr<OpenGlQuad> getGlComponent() { return bar_; }

    void resized() override {
        juce::ScrollBar::resized();
        bar_->setBounds(getLocalBounds());
        if (isVertical())
            bar_->setRounding(getWidth() * 0.25f);
        else
            bar_->setRounding(getHeight() * 0.25f);
    }

    void mouseEnter(const juce::MouseEvent &e) override {
        juce::ScrollBar::mouseEnter(e);
        bar_->setHover(true);
    }

    void mouseExit(const juce::MouseEvent &e) override {
        juce::ScrollBar::mouseExit(e);
        bar_->setHover(false);
    }

    void mouseDown(const juce::MouseEvent &e) override {
        juce::ScrollBar::mouseDown(e);
        bar_->setColor(color_.overlaidWith(color_));
    }

    void mouseUp(const juce::MouseEvent &e) override {
        juce::ScrollBar::mouseDown(e);
        bar_->setColor(color_);
    }

    void setColor(juce::Colour color) {
        color_ = color;
        bar_->setColor(color);
    }

    void setShrinkLeft(bool shrink_left) { bar_->setShrinkLeft(shrink_left); }

private:
    juce::Colour color_;
    std::shared_ptr<OpenGlScrollQuad> bar_;
};

class OpenGlCorners : public OpenGlMultiQuad {
public:
    OpenGlCorners() : OpenGlMultiQuad(4, Shaders::kRoundedCornerFragment) {
        setCoordinates(0, 1.0f, 1.0f, -1.0f, -1.0f);
        setCoordinates(1, 1.0f, 0.0f, -1.0f, 1.0f);
        setCoordinates(2, 0.0f, 0.0f, 1.0f, 1.0f);
        setCoordinates(3, 0.0f, 1.0f, 1.0f, -1.0f);
    }

    void setCorners(juce::Rectangle<int> bounds, float rounding) {
        float width = rounding / bounds.getWidth() * 2.0f;
        float height = rounding / bounds.getHeight() * 2.0f;

        setQuad(0, -1.0f, -1.0f, width, height);
        setQuad(1, -1.0f, 1.0f - height, width, height);
        setQuad(2, 1.0f - width, 1.0f - height, width, height);
        setQuad(3, 1.0f - width, -1.0f, width, height);
    }

    void setBottomCorners(juce::Rectangle<int> bounds, float rounding) {
        float width = rounding / bounds.getWidth() * 2.0f;
        float height = rounding / bounds.getHeight() * 2.0f;

        setQuad(0, -1.0f, -1.0f, width, height);
        setQuad(1, -2.0f, -2.0f, 0.0f, 0.0f);
        setQuad(2, -2.0f, -2.0f, 0.0f, 0.0f);
        setQuad(3, 1.0f - width, -1.0f, width, height);
    }
};
