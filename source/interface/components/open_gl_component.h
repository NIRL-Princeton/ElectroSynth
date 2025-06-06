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
#include "shaders.h"

#include "common.h"
#include "skin.h"
#include <juce_opengl/juce_opengl.h>

class SynthSection;
class OpenGlCorners;

class OpenGlComponent : public juce::Component
{
public:
  static bool setViewPort(juce::Component *component, juce::Rectangle<int> bounds, OpenGlWrapper &open_gl);
  static bool setViewPort(juce::Component *component, OpenGlWrapper &open_gl);
  static void setScissor(juce::Component *component, OpenGlWrapper &open_gl);
  static void setScissorBounds(juce::Component *component, juce::Rectangle<int> bounds, OpenGlWrapper &open_gl);

  static std::unique_ptr<juce::OpenGLShaderProgram::Uniform> getUniform(const OpenGlWrapper &open_gl,
                                                                        const juce::OpenGLShaderProgram &program,
                                                                        const char *name)
  {
    if (open_gl.context.extensions.glGetUniformLocation(program.getProgramID(), name) >= 0)
      return std::make_unique<juce::OpenGLShaderProgram::Uniform>(program, name);
    return nullptr;
  }

  static std::unique_ptr<juce::OpenGLShaderProgram::Attribute> getAttribute(const OpenGlWrapper &open_gl,
                                                                            const juce::OpenGLShaderProgram &program,
                                                                            const char *name)
  {
    if (open_gl.context.extensions.glGetAttribLocation(program.getProgramID(), name) >= 0)
      return std::make_unique<juce::OpenGLShaderProgram::Attribute>(program, name);
    return nullptr;
  }

  OpenGlComponent(juce::String name = "");
  virtual ~OpenGlComponent();

  virtual void resized() override;
  virtual void parentHierarchyChanged() override;

  void addRoundedCorners();
  void addBottomRoundedCorners();
  virtual void init(OpenGlWrapper &open_gl);
  virtual void render(OpenGlWrapper &open_gl, bool animate) = 0;
  void renderCorners(OpenGlWrapper &open_gl, bool animate, juce::Colour color, float rounding);
  void renderCorners(OpenGlWrapper &open_gl, bool animate);
  virtual void destroy(juce::OpenGLContext& open_gl);
  virtual bool isInit();
  virtual void paintBackground(juce::Graphics &g);
  void repaintBackground();

  juce::Colour getBodyColor() const { return body_color_; }

  void setParent(const SynthSection *parent) { parent_ = parent; }

  float findValue(Skin::ValueId value_id);
  void setSkinValues(const Skin &skin)
  {
    skin.setComponentColors(this, skin_override_, false);
  }
  void setSkinOverride(Skin::SectionOverride skin_override) { skin_override_ = skin_override; }
  static inline juce::String translateFragmentShader(const juce::String &code)
  {
#if OPENGL_ES
    return juce::String("#version 300 es\n") + "out mediump vec4 fragColor;\n" +
           code.replace("varying", "in").replace("texture2D", "texture").replace("gl_FragColor", "fragColor");
#else
    return juce::OpenGLHelpers::translateFragmentShaderToV3(code);
#endif
  }

  static inline juce::String translateVertexShader(const juce::String &code)
  {
#if OPENGL_ES
    return juce::String("#version 300 es\n") + code.replace("attribute", "in").replace("varying", "out");
#else
    return juce::OpenGLHelpers::translateVertexShaderToV3(code);
#endif
  }

  force_inline void checkGlError()
  {
#if DEBUG
    GLenum error = juce::gl::glGetError();
    assert(error == juce::gl::GL_NO_ERROR);
#endif
  }

  bool operator==(OpenGlComponent const &other) const
  {
    // Implement comparison logic based on your requirements
    return this->id == other.id; // Example comparison based on an 'id' member
  }
  void setBackgroundColor(const juce::Colour &color) { background_color_ = color; }

protected:
  bool setViewPort(OpenGlWrapper &open_gl);

  std::unique_ptr<OpenGlCorners> corners_;
  bool only_bottom_corners_;
  juce::Colour background_color_;
  juce::Colour body_color_;
  Skin::SectionOverride skin_override_;
  const SynthSection *parent_;

private:
  int id;
  static int nID;

  int generateID()
  {
    nID++;
    return nID;
  }

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlComponent)
};
