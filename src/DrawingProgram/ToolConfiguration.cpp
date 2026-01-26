#include "ToolConfiguration.hpp"

float ToolConfiguration::get_relative_width(float relativeWidthLocal) const {
    return globalConf.useGlobalRelativeWidth ? globalConf.relativeWidth : relativeWidthLocal;
}

void ToolConfiguration::relative_width_slider(GUIStuff::GUIManager& gui, const char* label, float* relativeWidthLocal) {
    gui.slider_scalar_field<float>("relstrokewidth", label, globalConf.useGlobalRelativeWidth ? &globalConf.relativeWidth : relativeWidthLocal, 3.0f, 40.0f);
}
