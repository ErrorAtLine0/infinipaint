#include "ToolConfiguration.hpp"
#include "DrawingProgram.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include "Helpers/FixedPoint.hpp"
#include <optional>
#include "Helpers/Logger.hpp"

float ToolConfiguration::get_relative_width(DrawingProgram& drawP, const WorldScalar& camInverseScale, float relativeWidthLocal) const {
    auto& lockedCameraScale = drawP.controls.lockedCameraScale;
    float relativeWidth = globalConf.useGlobalRelativeWidth ? globalConf.relativeWidth : relativeWidthLocal;
    if(lockedCameraScale.has_value()) {
        float lockMultiplier = static_cast<float>(WorldMultiplier(lockedCameraScale.value()) / WorldMultiplier(camInverseScale));
        if(lockMultiplier < 0.001f) {
            Logger::get().log("USERINFO", "Zoomed out too much, size unlocked");
            lockedCameraScale = std::nullopt;
            return relativeWidth;
        }
        else if(lockMultiplier > 1000.0f) {
            Logger::get().log("USERINFO", "Zoomed in too much, size unlocked");
            lockedCameraScale = std::nullopt;
            return relativeWidth;
        }
        return relativeWidth * lockMultiplier;
    }
    return relativeWidth;
}

void ToolConfiguration::relative_width_gui(DrawingProgram& drawP, const char* label, float* relativeWidthLocal) {
    auto& gui = drawP.world.main.toolbar.gui;
    auto& lockedCameraScale = drawP.controls.lockedCameraScale;
    gui.slider_scalar_field<float>("relstrokewidth", label, globalConf.useGlobalRelativeWidth ? &globalConf.relativeWidth : relativeWidthLocal, 3.0f, 40.0f);
    if(gui.text_button_wide("lock brush size", lockedCameraScale.has_value() ? "Unlock Brush Size" : "Lock Brush Size")) {
        if(lockedCameraScale.has_value())
            lockedCameraScale = std::nullopt;
        else
            lockedCameraScale = drawP.world.drawData.cam.c.inverseScale;
    }
}
