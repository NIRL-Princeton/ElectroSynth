#include "endpoint_arrow_component.h"

#include "ConnectionRecord.h"
#include "skin.h"

EndpointArrowComponent::EndpointArrowComponent(juce::String name) : PlainShapeComponent(std::move(name)) {
    setShape(Paths::rightArrow());
    setArrowScale(0.8f);
    setActive(true);
    setUseAlpha(true);
    setInterceptsMouseClicks(true, false);
}

void EndpointArrowComponent::mouseEnter(const juce::MouseEvent&) {
    mouse_hovered_ = true;
    updateArrowScale();
}

void EndpointArrowComponent::mouseExit(const juce::MouseEvent&) {
    mouse_hovered_ = false;
    updateArrowScale();
}

void EndpointArrowComponent::setMappingTarget(bool target) {
    mapping_target_ = target;
    updateArrowScale();
}

void EndpointArrowComponent::setDragTarget(bool target) {
    drag_target_ = target;
    updateArrowScale();
}

void EndpointArrowComponent::setArrowColor(juce::Colour color) {
    arrow_color_ = color;
    redrawImage(true);
}

void EndpointArrowComponent::clearArrowColor() {
    arrow_color_.reset();
    redrawImage(true);
}

void EndpointArrowComponent::render(OpenGlWrapper& open_gl, bool animate) {
    auto color = getArrowColor();
    if (drag_target_)
        color = color.brighter(0.75f);
    else if (mapping_target_)
        color = color.brighter(0.45f);

    setColor(color);
    PlainShapeComponent::render(open_gl, animate);
}

void EndpointArrowComponent::setArrowScale(float scale) {
    image().setTopLeft(-scale, scale);
    image().setTopRight(scale, scale);
    image().setBottomLeft(-scale, -scale);
    image().setBottomRight(scale, -scale);
}

void EndpointArrowComponent::updateArrowScale() {
    setArrowScale(drag_target_ || mouse_hovered_  ? 1.0f : mapping_target_ ? 0.9f : 0.8f);
}

juce::Colour EndpointArrowComponent::getArrowColor() const {
    return arrow_color_.value_or(findColour(Skin::kWidgetPrimary1, true));
}
