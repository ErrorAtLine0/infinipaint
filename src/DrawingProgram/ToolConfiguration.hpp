#pragma once
#include "../SharedTypes.hpp"
#include "Tools/ScreenshotTool.hpp"
#include "nlohmann/json.hpp"
#include "../GUIStuff/GUIManager.hpp"

class ToolConfiguration {
    public:
        struct BrushToolConfig {
            bool hasRoundCaps = true;
            float relativeWidth = 15.0f;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(BrushToolConfig, hasRoundCaps, relativeWidth)
        } brush;

        struct EraserToolConfig {
            float relativeWidth = 15.0f;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(EraserToolConfig, relativeWidth)
        } eraser;

        struct EllipseDrawToolConfig {
            float relativeWidth = 15.0f;
            int fillStrokeMode = 1;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(EllipseDrawToolConfig, relativeWidth, fillStrokeMode)
        } ellipseDraw;

        struct RectDrawToolConfig {
            float relativeWidth = 15.0f;
            float relativeRadiusWidth = 10.0f;
            int fillStrokeMode = 1;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(RectDrawToolConfig, relativeWidth, relativeRadiusWidth, fillStrokeMode)
        } rectDraw;

        struct EyeDropperToolConfig {
            bool selectingStrokeColor = true;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(EyeDropperToolConfig, selectingStrokeColor)
        } eyeDropper;

        struct LineDrawToolConfig {
            bool hasRoundCaps = true;
            float relativeWidth = 15.0f;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(LineDrawToolConfig, hasRoundCaps, relativeWidth)
        } lineDraw;

        struct ScreenshotToolConfig {
            int setDimensionSize = 1000;
            bool setDimensionIsX = true;
            ScreenshotTool::ScreenshotType selectedType = ScreenshotTool::ScreenshotType::SCREENSHOT_JPG;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ScreenshotToolConfig, setDimensionSize, setDimensionIsX, selectedType)
        } screenshot;

        struct GlobalConfig {
            Vector4f foregroundColor{1.0f, 1.0f, 1.0f, 1.0f};
            Vector4f backgroundColor{0.0f, 0.0f, 0.0f, 1.0f};
            float relativeWidth = 15.0f;
            bool useGlobalRelativeWidth = false;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GlobalConfig, useGlobalRelativeWidth, foregroundColor, backgroundColor, relativeWidth)
        } globalConf;

        enum class RelativeWidthFailCode {
            SUCCESS,
            TOO_ZOOMED_IN,
            TOO_ZOOMED_OUT
        };

        float& get_stroke_size_relative_width_ref(DrawingProgramToolType toolType);
        const float& get_stroke_size_relative_width_ref(DrawingProgramToolType toolType) const;
        std::pair<std::optional<float>, RelativeWidthFailCode> get_relative_width_from_value(DrawingProgram& drawP, const WorldScalar& camInverseScale, float relativeWidth) const;
        std::pair<std::optional<float>, RelativeWidthFailCode> get_relative_width_stroke_size(DrawingProgram& drawP, const WorldScalar& camInverseScale) const;
        void print_relative_width_fail_message(RelativeWidthFailCode failCode);
        void relative_width_gui(DrawingProgram& drawP, const char* label);

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ToolConfiguration, brush, eraser, ellipseDraw, rectDraw, eyeDropper, lineDraw, screenshot, globalConf)
};
