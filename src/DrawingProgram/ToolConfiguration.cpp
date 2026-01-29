#include "ToolConfiguration.hpp"
#include "DrawingProgram.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include "Helpers/FixedPoint.hpp"
#include <optional>
#include "Helpers/Logger.hpp"
#include "Tools/DrawingProgramToolBase.hpp"

float& ToolConfiguration::get_stroke_size_relative_width_ref(DrawingProgramToolType toolType) {
    if(globalConf.useGlobalRelativeWidth)
        return globalConf.relativeWidth;
    switch(toolType) {
        case DrawingProgramToolType::LINE:
            return lineDraw.relativeWidth;
        case DrawingProgramToolType::BRUSH:
            return brush.relativeWidth;
        case DrawingProgramToolType::ELLIPSE:
            return ellipseDraw.relativeWidth;
        case DrawingProgramToolType::RECTANGLE:
            return rectDraw.relativeWidth;
        case DrawingProgramToolType::ERASER:
            return eraser.relativeWidth;
        default:
            return globalConf.relativeWidth;
    }
    return globalConf.relativeWidth;
}

const float& ToolConfiguration::get_stroke_size_relative_width_ref(DrawingProgramToolType toolType) const {
    if(globalConf.useGlobalRelativeWidth)
        return globalConf.relativeWidth;
    switch(toolType) {
        case DrawingProgramToolType::LINE:
            return lineDraw.relativeWidth;
        case DrawingProgramToolType::BRUSH:
            return brush.relativeWidth;
        case DrawingProgramToolType::ELLIPSE:
            return ellipseDraw.relativeWidth;
        case DrawingProgramToolType::RECTANGLE:
            return rectDraw.relativeWidth;
        case DrawingProgramToolType::ERASER:
            return eraser.relativeWidth;
        default:
            return globalConf.relativeWidth;
    }
    return globalConf.relativeWidth;
}

std::pair<std::optional<float>, ToolConfiguration::RelativeWidthFailCode> ToolConfiguration::get_relative_width_from_value(DrawingProgram& drawP, const WorldScalar& camInverseScale, float relativeWidth) const {
    auto& lockedCameraScale = drawP.controls.lockedCameraScale;
    if(lockedCameraScale.has_value()) {
        float lockMultiplier = static_cast<float>(WorldMultiplier(lockedCameraScale.value()) / WorldMultiplier(camInverseScale));
        if(lockMultiplier < 0.001f)
            return {std::nullopt, RelativeWidthFailCode::TOO_ZOOMED_OUT};
        else if(lockMultiplier > 1000.0f)
            return {std::nullopt, RelativeWidthFailCode::TOO_ZOOMED_IN};
        return {relativeWidth * lockMultiplier, RelativeWidthFailCode::SUCCESS};
    }
    return {relativeWidth, RelativeWidthFailCode::SUCCESS};
}

std::pair<std::optional<float>, ToolConfiguration::RelativeWidthFailCode> ToolConfiguration::get_relative_width_stroke_size(DrawingProgram& drawP, const WorldScalar& camInverseScale) const {
    return get_relative_width_from_value(drawP, camInverseScale, get_stroke_size_relative_width_ref(drawP.drawTool->get_type()));
}

void ToolConfiguration::print_relative_width_fail_message(RelativeWidthFailCode failCode) {
    switch(failCode) {
        case RelativeWidthFailCode::SUCCESS:
            break;
        case RelativeWidthFailCode::TOO_ZOOMED_IN:
            Logger::get().log("USERINFO", "Zoomed in too much! Unlock size or zoom out");
            break;
        case RelativeWidthFailCode::TOO_ZOOMED_OUT:
            Logger::get().log("USERINFO", "Zoomed out too much! Unlock size or zoom in");
            break;
    }
}

void ToolConfiguration::relative_width_gui(DrawingProgram& drawP, const char* label) {
    auto& gui = drawP.world.main.toolbar.gui;
    auto& lockedCameraScale = drawP.controls.lockedCameraScale;
    gui.slider_scalar_field<float>("relstrokewidth", label, &get_stroke_size_relative_width_ref(drawP.drawTool->get_type()), 3.0f, 40.0f);
    if(gui.text_button_wide("lock brush size", lockedCameraScale.has_value() ? "Unlock Size" : "Lock Size to Zoom")) {
        if(lockedCameraScale.has_value())
            lockedCameraScale = std::nullopt;
        else
            lockedCameraScale = drawP.world.drawData.cam.c.inverseScale;
    }
}
