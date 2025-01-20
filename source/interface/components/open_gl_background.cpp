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

#include "open_gl_background.h"

#include "open_gl_component.h"
#include "common.h"
#include "look_and_feel/shaders.h"

OpenGlBackground::OpenGlBackground() : component_(nullptr),image_shader_(nullptr), vertices_() {
    new_background_ = false;
    vertex_buffer_ = 0;
    triangle_buffer_ = 0;
}

OpenGlBackground::~OpenGlBackground() { }

void OpenGlBackground::init(OpenGlWrapper& open_gl) {
    static const float vertices[] = {
            -1.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f
    };

    memcpy(vertices_, vertices, 16 * sizeof(float));

    static const int triangles[] = {
            0, 1, 2,
            2, 3, 0
    };

    open_gl.context.extensions.glGenBuffers(1, &vertex_buffer_);
    open_gl.context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vertex_buffer_);

    GLsizeiptr vert_size = static_cast<GLsizeiptr>(static_cast<size_t>(sizeof(vertices)));
    open_gl.context.extensions.glBufferData(juce::gl::GL_ARRAY_BUFFER, vert_size, vertices_, juce::gl::GL_STATIC_DRAW);

    open_gl.context.extensions.glGenBuffers(1, &triangle_buffer_);
    open_gl.context.extensions.glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, triangle_buffer_);

    GLsizeiptr tri_size = static_cast<GLsizeiptr>(static_cast<size_t>(sizeof(triangles)));
    open_gl.context.extensions.glBufferData(juce::gl::GL_ELEMENT_ARRAY_BUFFER, tri_size, triangles, juce::gl::GL_STATIC_DRAW);

    image_shader_ = open_gl.shaders->getShaderProgram(Shaders::kImageVertex, Shaders::kImageFragment);
    image_shader_->use();
    position_ = OpenGlComponent::getAttribute(open_gl, *image_shader_, "position");
    texture_coordinates_ = OpenGlComponent::getAttribute(open_gl, *image_shader_, "tex_coord_in");
    texture_uniform_ = OpenGlComponent::getUniform(open_gl, *image_shader_, "image");
}

void OpenGlBackground::destroy(OpenGlWrapper& open_gl) {
    if (background_.getWidth())
        background_.release();

    image_shader_ = nullptr;
    position_ = nullptr;
    texture_coordinates_ = nullptr;
    texture_uniform_ = nullptr;

    open_gl.context.extensions.glDeleteBuffers(1, &vertex_buffer_);
    open_gl.context.extensions.glDeleteBuffers(1, &triangle_buffer_);
}

void OpenGlBackground::bind(juce::OpenGLContext& open_gl_context) {
    open_gl_context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vertex_buffer_);
    open_gl_context.extensions.glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, triangle_buffer_);
    background_.bind();
}

void OpenGlBackground::enableAttributes(juce::OpenGLContext& open_gl_context) {
    if (position_ != nullptr) {
        open_gl_context.extensions.glVertexAttribPointer(position_->attributeID, 2, juce::gl::GL_FLOAT,
                                                         juce::gl::GL_FALSE, 4 * sizeof(float), nullptr);
        open_gl_context.extensions.glEnableVertexAttribArray(position_->attributeID);
    }
    if (texture_coordinates_ != nullptr) {
        open_gl_context.extensions.glVertexAttribPointer(texture_coordinates_->attributeID, 2, juce::gl::GL_FLOAT,
                                                         juce::gl::GL_FALSE, 4 * sizeof(float),
                                                         (GLvoid*)(2 * sizeof(float)));
        open_gl_context.extensions.glEnableVertexAttribArray(texture_coordinates_->attributeID);
    }
}

void OpenGlBackground::disableAttributes(juce::OpenGLContext& open_gl_context) {
    if (position_ != nullptr)
        open_gl_context.extensions.glDisableVertexAttribArray(position_->attributeID);

    if (texture_coordinates_ != nullptr)
        open_gl_context.extensions.glDisableVertexAttribArray(texture_coordinates_->attributeID);
}

void OpenGlBackground::render(OpenGlWrapper& open_gl) {
    mutex_.lock();
    if( component_!=nullptr)
    {
        OpenGlComponent::setViewPort(component_, component_->getLocalBounds(),open_gl) ;
    }
    if ((new_background_ || background_.getWidth() == 0) && background_image_.getWidth() > 0) {
        new_background_ = false;
        background_.loadImage(background_image_);
        DBG( background_.getWidth());
        float width_ratio = (1.0f * background_.getWidth()) / background_image_.getWidth();
        float height_ratio = (1.0f * background_.getHeight()) / background_image_.getHeight();
        float width_end = 2.0f * width_ratio - 1.0f;
        float height_end = 1.0f - 2.0f * height_ratio;

        vertices_[8] = vertices_[12] = width_end;
        vertices_[5] = vertices_[9] = height_end;

        open_gl.context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vertex_buffer_);
        GLsizeiptr vert_size = static_cast<GLsizeiptr>(static_cast<size_t>(16 * sizeof(float)));
        open_gl.context.extensions.glBufferData(juce::gl::GL_ARRAY_BUFFER, vert_size, vertices_, juce::gl::GL_STATIC_DRAW);
    }

    juce::gl::glDisable(juce::gl::GL_BLEND);
    juce::gl::glDisable(juce::gl::GL_SCISSOR_TEST);

    image_shader_->use();
    bind(open_gl.context);
    open_gl.context.extensions.glActiveTexture(juce::gl::GL_TEXTURE0);
    //DBG(background_.getWidth());
    if (texture_uniform_ != nullptr && background_.getWidth())
        texture_uniform_->set(0);

    enableAttributes(open_gl.context);
    juce::gl::glDrawElements(juce::gl::GL_TRIANGLES, 6, juce::gl::GL_UNSIGNED_INT, nullptr);
    disableAttributes(open_gl.context);
    background_.unbind();

    open_gl.context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
    open_gl.context.extensions.glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, 0);

    mutex_.unlock();
}

void OpenGlBackground::updateBackgroundImage(juce::Image background) {
    background_image_ = background;
    new_background_ = true;
}