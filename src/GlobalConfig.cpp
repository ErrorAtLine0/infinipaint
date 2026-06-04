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

#include "GlobalConfig.hpp"
#include <Helpers/Random.hpp>
#include "Helpers/StringHelpers.hpp"
#include "ResourceDisplay/ImageResourceDisplay.hpp"
#include "DrawingProgram/DrawingProgramCache.hpp"
#include <SDL3/SDL_time.h>

GlobalConfig::GlobalConfig() {
    load_default_palette();
    displayName = Random::get().alphanumeric_str(10);
    SDL_GetDateTimeLocalePreferences(&dateFormat, &timeFormat);
    if(dateFormat == SDL_DATE_FORMAT_YYYYMMDD) // Can't display this in a friendly way, so changing it for now
        dateFormat = SDL_DATE_FORMAT_DDMMYYYY;
    if(timeFormat == SDL_TIME_FORMAT_24HR)
        timeFormat = SDL_TIME_FORMAT_12HR; // For now, lock at 12 hour since it's the most used format, and time format detection doesn't work properly all the time
}

nlohmann::json GlobalConfig::get_config_json(const InputManager& input) const {
    using json = nlohmann::json;
    json toRet;

    json jKeybinds;
    for(unsigned i = 0; i < InputManager::KEY_ASSIGNABLE_COUNT; i++) {
        auto f = std::find_if(input.keyAssignments.begin(), input.keyAssignments.end(), [&](auto& p) {
            return p.second == i;
        });
        if(f != input.keyAssignments.end())
            jKeybinds[json(static_cast<InputManager::KeyCodeEnum>(i))] = input.key_assignment_to_str(f->first);
    }
    toRet["keybinds"] = jKeybinds;
    toRet["guiScale"] = guiScale;
    toRet["jumpTransitionTime"] = jumpTransitionTime;
    toRet["mainCallbackRate"] = mainCallbackRate;
    toRet["mainCallbackRateBackground"] = mainCallbackRateBackground;
    toRet["disableGraphicsDriverWorkarounds"] = disableGraphicsDriverWorkarounds;
    toRet["viewWebVersionWelcome"] = viewWebVersionWelcome;
    toRet["dragZoomSpeed"] = dragZoomSpeed;
    toRet["scrollZoomSpeed"] = scrollZoomSpeed;
    toRet["vsync"] = vsyncValue;
#ifndef __EMSCRIPTEN__
    toRet["applyDisplayScale"] = applyDisplayScale;
#endif
    toRet["displayName"] = displayName;
    toRet["antialiasing"] = antialiasing;
    toRet["useNativeFilePicker"] = useNativeFilePicker;
    toRet["themeInUse"] = themeCurrentlyLoaded;
    toRet["defaultCanvasBackgroundColor"] = defaultCanvasBackgroundColor;
    toRet["flipZoomToolDirection"] = flipZoomToolDirection;
    toRet["realTimeEraser"] = realTimeEraser;
#ifndef __EMSCRIPTEN__
    toRet["checkForUpdates"] = checkForUpdates;
#endif

    json tablet;
    tablet["brushPressureSmoothingFactor"] = tabletOptions.brushPressureSmoothingFactor;
    tablet["pressureAffectsBrushWidth"] = tabletOptions.pressureAffectsBrushWidth;
    tablet["middleClickButton"] = tabletOptions.middleClickButton;
    tablet["rightClickButton"] = tabletOptions.rightClickButton;
    tablet["ignoreMouseMovementWhenPenInProximity"] = tabletOptions.ignoreMouseMovementWhenPenInProximity;
    tablet["disableTouchWhenPenInProximity"] = tabletOptions.disableTouchWhenPenInProximity;
    tablet["brushMinimumSize"] = tabletOptions.brushMinimumSize;
    tablet["zoomWhilePenDownAndButtonHeld"] = tabletOptions.zoomWhilePenDownAndButtonHeld;
    toRet["tablet"] = tablet;

    json debugJson;
    debugJson["mobileUI"] = mobileUI;
    debugJson["jumpTransitionEasing"] = jumpTransitionEasing;
    debugJson["imageLoadMaxThreads"] = ImageResourceDisplay::IMAGE_LOAD_THREAD_COUNT_MAX;
    debugJson["cacheNodeResolution"] = DrawingProgramCache::CACHE_NODE_RESOLUTION;
    debugJson["maxCacheNodes"] = DrawingProgramCache::MAXIMUM_DRAW_CACHE_SURFACES;
    debugJson["maxComponentsInNode"] = DrawingProgramCache::MAXIMUM_COMPONENTS_IN_SINGLE_NODE;
    debugJson["componentCountToForceCacheRebuild"] = DrawingProgramCache::MINIMUM_COMPONENTS_TO_START_REBUILD;
    debugJson["maximumFrameTimeToForceCacheRebuild"] = DrawingProgramCache::MILLISECOND_FRAME_TIME_TO_FORCE_CACHE_REFRESH;
    debugJson["millisecondMinimumTimeToCheckForCacheRebuild"] = DrawingProgramCache::MILLISECOND_MINIMUM_TIME_TO_CHECK_FORCE_REFRESH;
    toRet["debug"] = debugJson;

    return toRet;
}

void GlobalConfig::set_config_json(InputManager& input, const nlohmann::json& j, VersionNumber version) {
    using json = nlohmann::json;
    input.keyAssignments.clear();
    try {
        const json& jKeybinds = j.at("keybinds");
        for(unsigned i = 0; i < InputManager::KEY_ASSIGNABLE_COUNT; i++) {
            try {
                Vector2ui32 a = input.key_assignment_from_str(jKeybinds.at(json(static_cast<InputManager::KeyCodeEnum>(i))));
                if(a != Vector2ui32{0, 0})
                    input.keyAssignments.emplace(a, i);
                else
                    throw;
            }
            catch(...) {
                auto f = std::find_if(input.defaultKeyAssignments.begin(), input.defaultKeyAssignments.end(), [&](auto& p) {
                    return p.second == i;
                });
                input.keyAssignments.emplace(f->first, i);
            }
        }
    }
    catch(...) {
        input.keyAssignments = input.defaultKeyAssignments;
    }
    try{j.at("displayName").get_to(displayName);} catch(...) {}
    try{j.at("dragZoomSpeed").get_to(dragZoomSpeed);} catch(...) {}
    try{j.at("scrollZoomSpeed").get_to(scrollZoomSpeed);} catch(...) {}
    try{j.at("mainCallbackRate").get_to(mainCallbackRate);} catch(...) {}
    try{j.at("mainCallbackRateBackground").get_to(mainCallbackRateBackground);} catch(...) {}
    if(version >= VersionNumber(0, 6, 0))
        try{j.at("vsync").get_to(vsyncValue);} catch(...) {}
#ifndef __EMSCRIPTEN__
    try{j.at("applyDisplayScale").get_to(applyDisplayScale);} catch(...) {}
#endif
    try{j.at("guiScale").get_to(guiScale);} catch(...) {}
    try{j.at("jumpTransitionTime").get_to(jumpTransitionTime);} catch(...) {}
    try{j.at("disableGraphicsDriverWorkarounds").get_to(disableGraphicsDriverWorkarounds);} catch(...) {}
    try{j.at("viewWebVersionWelcome").get_to(viewWebVersionWelcome);} catch(...) {}
    try{j.at("useNativeFilePicker").get_to(useNativeFilePicker);} catch(...) {}
    try{j.at("themeInUse").get_to(themeCurrentlyLoaded);} catch(...) {}
    if(version >= VersionNumber(0, 3, 0))
        try{j.at("defaultCanvasBackgroundColor").get_to(defaultCanvasBackgroundColor);} catch(...) {}
    try{j.at("flipZoomToolDirection").get_to(flipZoomToolDirection);} catch(...) {}
    try{j.at("realTimeEraser").get_to(realTimeEraser);} catch(...) {}
#ifndef __EMSCRIPTEN__
    try{j.at("checkForUpdates").get_to(checkForUpdates);} catch(...) {}
#endif
    try{j.at("antialiasing").get_to(antialiasing);} catch(...) {}  

    try{j.at("tablet").at("brushPressureSmoothingFactor").get_to(tabletOptions.brushPressureSmoothingFactor);} catch(...) {}
    try{j.at("tablet").at("pressureAffectsBrushWidth").get_to(tabletOptions.pressureAffectsBrushWidth);} catch(...) {}
    try{j.at("tablet").at("middleClickButton").get_to(tabletOptions.middleClickButton);} catch(...) {}
    try{j.at("tablet").at("rightClickButton").get_to(tabletOptions.rightClickButton);} catch(...) {}
    try{j.at("tablet").at("ignoreMouseMovementWhenPenInProximity").get_to(tabletOptions.ignoreMouseMovementWhenPenInProximity);} catch(...) {}
    try{j.at("tablet").at("disableTouchWhenPenInProximity").get_to(tabletOptions.disableTouchWhenPenInProximity);} catch(...) {}
    try{j.at("tablet").at("brushMinimumSize").get_to(tabletOptions.brushMinimumSize);} catch(...) {}
    try{j.at("tablet").at("zoomWhilePenDownAndButtonHeld").get_to(tabletOptions.zoomWhilePenDownAndButtonHeld);} catch(...) {}

    try{j.at("debug").at("mobileUI").get_to(mobileUI);} catch(...) {}
    try{j.at("debug").at("jumpTransitionEasing").get_to(jumpTransitionEasing);} catch(...) {}
    try{j.at("debug").at("imageLoadMaxThreads").get_to(ImageResourceDisplay::IMAGE_LOAD_THREAD_COUNT_MAX);} catch(...) {}
    try{j.at("debug").at("cacheNodeResolution").get_to(DrawingProgramCache::CACHE_NODE_RESOLUTION);} catch(...) {}
    try{j.at("debug").at("maxCacheNodes").get_to(DrawingProgramCache::MAXIMUM_DRAW_CACHE_SURFACES);} catch(...) {}
    try{j.at("debug").at("maxComponentsInNode").get_to(DrawingProgramCache::MAXIMUM_COMPONENTS_IN_SINGLE_NODE);} catch(...) {}
    try{j.at("debug").at("componentCountToForceCacheRebuild").get_to(DrawingProgramCache::MINIMUM_COMPONENTS_TO_START_REBUILD);} catch(...) {}
    try{j.at("debug").at("maximumFrameTimeToForceCacheRebuild").get_to(DrawingProgramCache::MILLISECOND_FRAME_TIME_TO_FORCE_CACHE_REFRESH);} catch(...) {}
    try{j.at("debug").at("millisecondMinimumTimeToCheckForCacheRebuild").get_to(DrawingProgramCache::MILLISECOND_MINIMUM_TIME_TO_CHECK_FORCE_REFRESH);} catch(...) {}
}

void GlobalConfig::save_palettes() {
    nlohmann::json j;
    auto palettesToSave = palettes;
    palettesToSave.erase(palettesToSave.begin());
    nlohmann::to_json(j, palettesToSave);
    std::string saveJson = nlohmann::to_string(j);
    SDL_SaveFile((configPath / "palettes.json").string().c_str(), saveJson.c_str(), saveJson.size());
}

void GlobalConfig::load_palettes() {
    load_default_palette();
    using json = nlohmann::json;
    try {
        json j(nlohmann::json::parse(read_file_to_string(configPath / "palettes.json")));
        std::vector<Palette> p;
        j.get_to(p);
        palettes.insert(palettes.end(), p.begin(), p.end());
    } catch(...) {}
}

void GlobalConfig::load_default_palette() {
    palettes.clear();
    palettes.emplace_back();
    auto& palette = palettes.back().colors;
    palettes.back().name = "Default";
    palette = {{1.0,1.0,1.0},{0.0,0.0,0.0},{1.0,0.0,0.0},{1.0,0.529411792755127,0.0},{1.0,0.8274509906768799,0.0},{0.8705882430076599,1.0,0.03921568766236305},{0.6313725709915161,1.0,0.03921568766236305},{0.03921568766236305,1.0,0.6000000238418579},{0.03921568766236305,0.9372549057006836,1.0},{0.0784313753247261,0.4901960790157318,0.9607843160629272},{0.3450980484485626,0.03921568766236305,1.0},{0.7450980544090271,0.03921568766236305,1.0}};
}

void GlobalConfig::load_licenses() {
    {
        int globCount;
        std::filesystem::path third_party_license_path("data/third_party_licenses");
        char** filesInPath = SDL_GlobDirectory(third_party_license_path.string().c_str(), nullptr, 0, &globCount);
        if(filesInPath) {
            for(int i = 0; i < globCount; i++) {
                std::filesystem::path filePath = third_party_license_path / std::filesystem::path(filesInPath[i]);
                SDL_PathInfo fileInfo;
                if(SDL_GetPathInfo(filePath.string().c_str(), &fileInfo) && fileInfo.type == SDL_PATHTYPE_FILE) {
                    thirdPartyLicenses.emplace_back(filePath.filename().string(), read_file_to_string(filePath));
                }
            }
            SDL_free(filesInPath);
        }
    }

    std::sort(thirdPartyLicenses.begin(), thirdPartyLicenses.end(), [](const auto& a1, const auto& a2) {
        return std::lexicographical_compare(a1.first.begin(), a1.first.end(), a2.first.begin(), a2.first.end());
    });
    ownLicenseText = "InfiniPaint v" + VersionConstants::CURRENT_VERSION_STRING + "\n";
    ownLicenseText += read_file_to_string("data/license");
}
