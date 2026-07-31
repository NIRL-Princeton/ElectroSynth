#pragma once

#include "open_gl_image_component.h"
#include "paths.h"

class EndpointArrowComponent : public PlainShapeComponent {
public:
    explicit EndpointArrowComponent(juce::String name);

    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    void setMappingTarget(bool target);
    void setDragTarget(bool target);
    void setArrowColor(juce::Colour color);
    void clearArrowColor();
    juce::Colour getArrowColor() const;

    void render(OpenGlWrapper& open_gl, bool animate) override;

protected:
    void setArrowScale(float scale);
    void updateArrowScale();

private:
    std::optional<juce::Colour> arrow_color_;
    bool mouse_hovered_ = false;
    bool mapping_target_ = false;
    bool drag_target_ = false;
};
