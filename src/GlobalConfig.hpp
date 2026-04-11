#pragma once
#include <nlohmann/json.hpp>
#include <Helpers/VersionNumber.hpp>
#include "InputManager.hpp"

#define DEFAULT_CANVAS_BACKGROUND_COLOR Vector3f{0.07f, 0.07f, 0.07f}

class GlobalConfig {
    public:
        GlobalConfig();

        float guiScale = 1.0f;

        double dragZoomSpeed = 0.02;
        double scrollZoomSpeed = 0.4;
        float jumpTransitionTime = 0.5f;
        Vector4f jumpTransitionEasing{0.75, 0.25, 0.25, 0.75};
        bool viewWebVersionWelcome = false;

        struct TabletOptions {
            bool pressureAffectsBrushWidth = true;
            float smoothingSamplingTime = 0.04f;
            uint8_t middleClickButton = 1;
            uint8_t rightClickButton = 2;
            bool ignoreMouseMovementWhenPenInProximity = false;
            float brushMinimumSize = 0.0f;
            bool zoomWhilePenDownAndButtonHeld = true;
        } tabletOptions;

        Vector3f defaultCanvasBackgroundColor = DEFAULT_CANVAS_BACKGROUND_COLOR;

        nlohmann::json get_config_json(const InputManager& input) const;
        void set_config_json(InputManager& input, const nlohmann::json& j, VersionNumber version);

        bool showPerformance = false;
        bool checkForUpdates = false;
        bool useNativeFilePicker = true;

        std::string themeCurrentlyLoaded = "Default";

        enum class AntiAliasing {
            NONE,
            SKIA,
            DYNAMIC_MSAA
        } antialiasing = AntiAliasing::SKIA;

        std::string displayName;
        bool flipZoomToolDirection = false;

        bool disableGraphicsDriverWorkarounds = false;
        int vsyncValue = 1;

        #ifndef __EMSCRIPTEN__
            bool applyDisplayScale = true;
        #endif
};

NLOHMANN_JSON_SERIALIZE_ENUM(GlobalConfig::AntiAliasing, {
    {GlobalConfig::AntiAliasing::NONE, "None"},
    {GlobalConfig::AntiAliasing::SKIA, "Skia"},
    {GlobalConfig::AntiAliasing::DYNAMIC_MSAA, "Dynamic MSAA"},
})
