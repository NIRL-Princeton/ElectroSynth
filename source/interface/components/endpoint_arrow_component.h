#pragma once

#include "open_gl_image_component.h"
#include "paths.h"
#include "ConnectionRecord.h"

class ConnectionSlots;

class EndpointArrowComponent : public PlainShapeComponent {
public:
    explicit EndpointArrowComponent(juce::String name, electrosynth::EndpointDescriptor endpoint = {});

    const electrosynth::EndpointDescriptor& getEndpoint() const noexcept {
        return endpoint_;
    }

    bool hasEndpoint() const noexcept {
        return endpoint_.address.isValid();
    }

    void setConnectionSlots(ConnectionSlots* slots) noexcept {
        connection_slots_ = slots;
    }

    ConnectionSlots* getConnectionSlots() const noexcept {
        return connection_slots_;
    }

    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void resized() override;

    void setMappingTarget(bool target);
    void setDragTarget(bool target);
    void setArrowColor(juce::Colour color);
    juce::Colour getArrowColor() const;

    void render(OpenGlWrapper& open_gl, bool animate) override;

protected:
    void setArrowScale(float scale);
    void updateArrowScale();

private:
    electrosynth::EndpointDescriptor endpoint_;
    ConnectionSlots* connection_slots_ = nullptr;
    std::optional<juce::Colour> arrow_color_;
    bool mouse_hovered_ = false;
    bool mapping_target_ = false;
    bool drag_target_ = false;
};
