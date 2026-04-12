#include "GlobalConfig.hpp"
#include <Helpers/Random.hpp>
#include "ResourceDisplay/ImageResourceDisplay.hpp"
#include "DrawingProgram/DrawingProgramCache.hpp"
#include <fstream>

GlobalConfig::GlobalConfig() {
    load_default_palette();
    displayName = Random::get().alphanumeric_str(10);
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
    toRet["disableGraphicsDriverWorkarounds"] = disableGraphicsDriverWorkarounds;
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
#ifndef __EMSCRIPTEN__
    toRet["checkForUpdates"] = checkForUpdates;
#endif

    json tablet;
    tablet["pressureAffectsBrushWidth"] = tabletOptions.pressureAffectsBrushWidth;
    tablet["smoothingSamplingTime"] = tabletOptions.smoothingSamplingTime;
    tablet["middleClickButton"] = tabletOptions.middleClickButton;
    tablet["rightClickButton"] = tabletOptions.rightClickButton;
    tablet["ignoreMouseMovementWhenPenInProximity"] = tabletOptions.ignoreMouseMovementWhenPenInProximity;
    tablet["brushMinimumSize"] = tabletOptions.brushMinimumSize;
    tablet["zoomWhilePenDownAndButtonHeld"] = tabletOptions.zoomWhilePenDownAndButtonHeld;
    toRet["tablet"] = tablet;

    json debugJson;
    debugJson["showPerformance"] = showPerformance;
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
    try{j.at("vsync").get_to(vsyncValue);} catch(...) {}
#ifndef __EMSCRIPTEN__
    try{j.at("applyDisplayScale").get_to(applyDisplayScale);} catch(...) {}
#endif
    try{j.at("guiScale").get_to(guiScale);} catch(...) {}
    try{j.at("jumpTransitionTime").get_to(jumpTransitionTime);} catch(...) {}
    try{j.at("disableGraphicsDriverWorkarounds").get_to(disableGraphicsDriverWorkarounds);} catch(...) {}
    try{j.at("useNativeFilePicker").get_to(useNativeFilePicker);} catch(...) {}
    try{j.at("themeInUse").get_to(themeCurrentlyLoaded);} catch(...) {}
    if(version >= VersionNumber(0, 3, 0))
        try{j.at("defaultCanvasBackgroundColor").get_to(defaultCanvasBackgroundColor);} catch(...) {}
    try{j.at("flipZoomToolDirection").get_to(flipZoomToolDirection);} catch(...) {}
#ifndef __EMSCRIPTEN__
    try{j.at("checkForUpdates").get_to(checkForUpdates);} catch(...) {}
#endif
    try{j.at("antialiasing").get_to(antialiasing);} catch(...) {}  

    try{j.at("tablet").at("pressureAffectsBrushWidth").get_to(tabletOptions.pressureAffectsBrushWidth);} catch(...) {}
    try{j.at("tablet").at("smoothingSamplingTime").get_to(tabletOptions.smoothingSamplingTime);} catch(...) {}
    try{j.at("tablet").at("middleClickButton").get_to(tabletOptions.middleClickButton);} catch(...) {}
    try{j.at("tablet").at("rightClickButton").get_to(tabletOptions.rightClickButton);} catch(...) {}
    try{j.at("tablet").at("ignoreMouseMovementWhenPenInProximity").get_to(tabletOptions.ignoreMouseMovementWhenPenInProximity);} catch(...) {}
    try{j.at("tablet").at("brushMinimumSize").get_to(tabletOptions.brushMinimumSize);} catch(...) {}
    try{j.at("tablet").at("zoomWhilePenDownAndButtonHeld").get_to(tabletOptions.zoomWhilePenDownAndButtonHeld);} catch(...) {}

    try{j.at("debug").at("showPerformance").get_to(showPerformance);} catch(...) {}  
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
    std::ofstream f(configPath / "palettes.json");
    if(f.is_open()) {
        using json = nlohmann::json;
        json j;
        auto palettesToSave = palettes;
        palettesToSave.erase(palettesToSave.begin());
        j = palettesToSave;
        f << j;
        f.close();
    }
}

void GlobalConfig::load_palettes() {
    std::ifstream f(configPath / "palettes.json");
    load_default_palette();
    if(f.is_open()) {
        using json = nlohmann::json;
        try {
            json j;
            f >> j;
            std::vector<Palette> palettes;
            j.get_to(palettes);
            palettes.insert(palettes.end(), palettes.begin(), palettes.end());
        } catch(...) {}
        f.close();
    }
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
    ownLicenseText = "InfiniPaint v" + VersionConstants::CURRENT_VERSION_STRING;
    ownLicenseText +=
R"(

Copyright © 2026 Yousef Khadadeh

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
)";
}

