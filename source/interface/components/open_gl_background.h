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

class OpenGlBackground {
public:
    OpenGlBackground();
    virtual ~OpenGlBackground();

    void updateBackgroundImage(juce::Image background);
    virtual void init(OpenGlWrapper& open_gl);
    virtual void render(OpenGlWrapper& open_gl);
    virtual void destroy(juce::OpenGLContext& open_gl);

    void lock() { mutex_.lock(); }
    void unlock() { mutex_.unlock(); }

    juce::OpenGLShaderProgram* shader() { return image_shader_; }
    juce::OpenGLShaderProgram::Uniform* texture_uniform() { return texture_uniform_.get(); }

    void bind(juce::OpenGLContext& open_gl_context);
    void enableAttributes(juce::OpenGLContext& open_gl_context);
    void disableAttributes(juce::OpenGLContext& open_gl_context);
    void setComponent(juce::Component* component) { component_ = component; }

private:
    juce::OpenGLShaderProgram* image_shader_;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> texture_uniform_;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> position_;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> texture_coordinates_;

    float vertices_[16];

    std::mutex mutex_;
    juce::OpenGLTexture background_;
    bool new_background_;
    juce::Image background_image_;

    GLuint vertex_buffer_;
    GLuint triangle_buffer_;
    juce::Component* component_;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlBackground)
};
