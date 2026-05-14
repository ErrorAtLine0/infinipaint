/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <SDL3/SDL_time.h>
#include <nlohmann/json.hpp>
#include <Helpers/VersionNumber.hpp>
#include "InputManager.hpp"

#define DEFAULT_CANVAS_BACKGROUND_COLOR Vector3f{0.07f, 0.07f, 0.07f}

class GlobalConfig {
    public:
        GlobalConfig();

        std::filesystem::path currentSearchPath;

        std::string ownLicenseText;
        std::vector<std::pair<std::string, std::string>> thirdPartyLicenses;

        struct Palette {
            std::string name;
            std::vector<Vector3f> colors;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Palette, name, colors)
        };

        std::vector<Palette> palettes;

        std::filesystem::path configPath;

        #ifdef __ANDROID__
            float guiScale = 1.2f;
        #else
            float guiScale = 1.0f;
        #endif

        double dragZoomSpeed = 0.02;
        double scrollZoomSpeed = 0.4;
        float jumpTransitionTime = 0.5f;
        Vector4f jumpTransitionEasing{0.75, 0.25, 0.25, 0.75};
#ifdef __EMSCRIPTEN__
        bool viewWebVersionWelcome = true;
#else
        bool viewWebVersionWelcome = false;
#endif

#ifdef __ANDROID__
        bool mobileUI = true;
#else
        bool mobileUI = false;
#endif

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

        void save_palettes();
        void load_palettes();
        void load_licenses();

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

        SDL_DateFormat dateFormat = SDL_DATE_FORMAT_DDMMYYYY;
        SDL_TimeFormat timeFormat = SDL_TIME_FORMAT_12HR;

        #ifndef __EMSCRIPTEN__
            bool applyDisplayScale = true;
        #endif
    private:
        void load_default_palette();
};

NLOHMANN_JSON_SERIALIZE_ENUM(GlobalConfig::AntiAliasing, {
    {GlobalConfig::AntiAliasing::NONE, "None"},
    {GlobalConfig::AntiAliasing::SKIA, "Skia"},
    {GlobalConfig::AntiAliasing::DYNAMIC_MSAA, "Dynamic MSAA"},
})
