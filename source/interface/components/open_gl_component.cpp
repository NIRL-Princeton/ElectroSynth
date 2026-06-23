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

#include "open_gl_component.h"

#include "open_gl_multi_quad.h"
#include "fullInterface.h"

//open_gl_component.cpp is the base class for OpenGL-rendered UI components. It inherits from juce::Component and
// defines the common OpenGL lifecycle: init(), render(), destroy(), isInit().
// It also handles viewport/scissor setup by translating JUCE component bounds into OpenGL coordinates,
// accounting for display scale, rendering scale, and the app’s resizing scale. It has helpers for shader uniforms/attributes,
// background repainting, skin lookup, rounded corners, and debug drawing.

namespace {

  juce::Rectangle<int> getGlobalBounds(juce::Component* component, juce::Rectangle<int> bounds) {
    juce::Component* parent = component->getParentComponent();
    while (parent && dynamic_cast<FullInterface*>(component) == nullptr) {
      bounds = bounds + component->getPosition();
      component = parent;
      parent = component->getParentComponent();
    }

    return bounds;
  }

  juce::Rectangle<int> getGlobalVisibleBounds(juce::Component* component, juce::Rectangle<int> visible_bounds) {
    juce::Component* parent = component->getParentComponent();
    while (parent && dynamic_cast<FullInterface*>(parent) == nullptr) {
      visible_bounds = visible_bounds + component->getPosition();
      parent->getLocalBounds().intersectRectangle(visible_bounds);
      component = parent;
      parent = component->getParentComponent();
    }
    return visible_bounds + component->getPosition();
  }
}

void OpenGlComponent::drawDebugBox(int x, int y, int width, int height, int viewportHeight) {
  {
    GLint viewport[4];
    juce::gl::glGetIntegerv(juce::gl::GL_VIEWPORT, viewport);
    int viewportHeight = viewport[3];

    int flippedY = viewportHeight - y - height;

    // Set up orthographic projection (2D)
    juce::gl::glMatrixMode(juce::gl::GL_PROJECTION);
    juce::gl::glPushMatrix();
    juce::gl::glLoadIdentity();
    juce::gl::glOrtho(0, viewport[2], 0, viewport[3], -1.0, 1.0);

    juce::gl::glMatrixMode(juce::gl::GL_MODELVIEW);
    juce::gl::glPushMatrix();
    juce::gl::glLoadIdentity();

    // Temporarily disable scissor to draw debug overlay
    juce::gl::glDisable(juce::gl::GL_SCISSOR_TEST);
    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE_MINUS_SRC_ALPHA);
    juce::gl::glColor4f(1.0f, 0.0f, 0.0f, 0.3f);  // Red, 30% transparent

    juce::gl::glBegin(juce::gl::GL_QUADS);
    juce::gl::glVertex2f((GLfloat)x,               (GLfloat)flippedY);
    juce::gl::glVertex2f((GLfloat)(x + width),     (GLfloat)flippedY);
    juce::gl::glVertex2f((GLfloat)(x + width),     (GLfloat)(flippedY + height));
    juce::gl::glVertex2f((GLfloat)x,               (GLfloat)(flippedY + height));
    juce::gl::glEnd();

    juce::gl::glDisable(juce::gl::GL_BLEND);
    juce::gl::glEnable(juce::gl::GL_SCISSOR_TEST);  // Restore

    // Restore previous projection/modelview matrices
    juce::gl::glPopMatrix(); // Modelview
    juce::gl::glMatrixMode(juce::gl::GL_PROJECTION);
    juce::gl::glPopMatrix();
    juce::gl::glMatrixMode(juce::gl::GL_MODELVIEW);
  }
}

int OpenGlComponent::nID = 0;
OpenGlComponent::OpenGlComponent(juce::String name) : juce::Component(name), only_bottom_corners_(false),
                                                parent_(nullptr), skin_override_(Skin::kNone), scissor_component_(nullptr) {
  background_color_ = juce::Colours::black;
  id = generateID();
}

OpenGlComponent::~OpenGlComponent() {
}

bool OpenGlComponent::setViewPort(juce::Component* component, juce::Rectangle<int> bounds, OpenGlWrapper& open_gl) {

    FullInterface* top_level = component->findParentComponentOfClass<FullInterface>();
    if(top_level == nullptr)
      return false;
    float scale = open_gl.display_scale;
    float resize_scale = top_level->getResizingScale();
    float render_scale = 1.0f;
    if (scale == 1.0f) render_scale *= open_gl.context.getRenderingScale();
    float gl_scale = render_scale * resize_scale;

    juce::Rectangle<int> top_level_bounds = top_level->getBounds();
    juce::Rectangle<int> global_bounds = getGlobalBounds(component, bounds);
    juce::Rectangle<int> visible_bounds = getGlobalVisibleBounds(component, bounds);

    float comp_scale = Component::getApproximateScaleFactorForComponent(component);
//  //juce::gl::glViewport(gl_scale * global_bounds.getX(),
//             std::ceil(scale * render_scale * top_level_bounds.getHeight()) - gl_scale * global_bounds.getBottom(),
//             gl_scale * global_bounds.getWidth(), gl_scale * global_bounds.getHeight());
    juce::gl::glViewport(gl_scale * scale * global_bounds.getX(),
      (std::ceil(scale * render_scale * top_level_bounds.getHeight()) - gl_scale *  scale  * global_bounds.getBottom()),
      scale * gl_scale * global_bounds.getWidth(), scale * gl_scale * global_bounds.getHeight());

    if (visible_bounds.getWidth() <= 0 || visible_bounds.getHeight() <= 0) return false;

    juce::gl::glScissor(gl_scale * scale * global_bounds.getX(),
      (std::ceil(scale * render_scale * top_level_bounds.getHeight()) - gl_scale *  scale  * global_bounds.getBottom()),
      comp_scale * scale * gl_scale * global_bounds.getWidth(),comp_scale* scale * gl_scale * global_bounds.getHeight());

    return true;
}

bool OpenGlComponent::setViewPort(juce::Component* component, OpenGlWrapper& open_gl) {
  return setViewPort(component, component->getLocalBounds(), open_gl);
}

bool OpenGlComponent::setViewPort(OpenGlWrapper& open_gl) {
  return setViewPort(this, open_gl);
}

void OpenGlComponent::setScissor(juce::Component* component, OpenGlWrapper& open_gl) {
  setScissorBounds(component, component->getLocalBounds(), open_gl);
}

void OpenGlComponent::setScissorBounds(juce::Component* component, juce::Rectangle<int> bounds, OpenGlWrapper& open_gl) {
  if (component == nullptr)
    return;

  FullInterface* top_level = component->findParentComponentOfClass<FullInterface>();
  if (top_level == nullptr) return;
  float scale = open_gl.display_scale;
  float resize_scale = top_level->getResizingScale();
  float render_scale = 1.0f;
  if (scale == 1.0f)
    render_scale *= open_gl.context.getRenderingScale();

  float gl_scale = render_scale * resize_scale;

  juce::Rectangle<int> top_level_bounds = top_level->getBounds();
  juce::Rectangle<int> visible_bounds = getGlobalVisibleBounds(component, bounds);

  if (visible_bounds.getHeight() > 0 && visible_bounds.getWidth() > 0) {
    juce::gl::glScissor(gl_scale * visible_bounds.getX(),
              std::ceil(scale * render_scale * top_level_bounds.getHeight()) - gl_scale * visible_bounds.getBottom(),
              gl_scale * visible_bounds.getWidth(), gl_scale * visible_bounds.getHeight());
  }
}

void OpenGlComponent::paintBackground(juce::Graphics& g) {
  if (!isVisible()) return;
  g.fillAll(findColour(Skin::kWidgetBackground, true));
}

void OpenGlComponent::repaintBackground() {
  if (!isShowing()) return;

  FullInterface* parent = findParentComponentOfClass<FullInterface>();
  if (parent) parent->repaintOpenGlBackground(this);
}

void OpenGlComponent::resized() {

    if (corners_) corners_->setBounds(getLocalBounds());
    body_color_ = findColour (Skin::kBody, true);
}

void OpenGlComponent::parentHierarchyChanged() {
//  if (num_voices_readout_ == nullptr) {
//    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
//    if (parent)
//      num_voices_readout_ = parent->getSynth()->getStatusOutput("num_voices");
//  }

  juce::Component::parentHierarchyChanged();
}

void OpenGlComponent::addRoundedCorners() {
  corners_ = std::make_unique<OpenGlCorners>();
  addAndMakeVisible(corners_.get());
}

void OpenGlComponent::addBottomRoundedCorners() {
  only_bottom_corners_ = true;
  addRoundedCorners();
}

void OpenGlComponent::init(OpenGlWrapper& open_gl) {
  if (corners_) corners_->init(open_gl);

}

void OpenGlComponent::renderCorners(OpenGlWrapper& open_gl, bool animate, juce::Colour color, float rounding) {
  if (corners_) {
    if (only_bottom_corners_)
      corners_->setBottomCorners(getLocalBounds(), rounding);
    else
      corners_->setCorners(getLocalBounds(), rounding);
    corners_->setColor(color);
    corners_->render(open_gl, animate);
  }
}

void OpenGlComponent::renderCorners(OpenGlWrapper& open_gl, bool animate) {
  renderCorners(open_gl, animate, body_color_, findValue(Skin::kWidgetRoundedCorner));
}

void OpenGlComponent::destroy(juce::OpenGLContext& open_gl) {
    if (corners_) corners_->destroy(open_gl);
}

bool OpenGlComponent::isInit() {
    if (corners_) return corners_->shader() != nullptr;
}

float OpenGlComponent::findValue(Skin::ValueId value_id) {
    if (parent_) return parent_->findValue(value_id);

    _ASSERT(false);
    return 0.0f;
}
