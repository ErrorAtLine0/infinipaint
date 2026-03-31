#include "GlobalConfig.hpp"
#include <Helpers/Random.hpp>
#include "ResourceDisplay/ImageResourceDisplay.hpp"
#include "DrawingProgram/DrawingProgramCache.hpp"

GlobalConfig::GlobalConfig() {
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
    debugJson["fpsLimit"] = fpsLimit;
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
    try{j.at("debug").at("fpsLimit").get_to(fpsLimit);} catch(...) {}
    try{j.at("debug").at("jumpTransitionEasing").get_to(jumpTransitionEasing);} catch(...) {}
    try{j.at("debug").at("imageLoadMaxThreads").get_to(ImageResourceDisplay::IMAGE_LOAD_THREAD_COUNT_MAX);} catch(...) {}
    try{j.at("debug").at("cacheNodeResolution").get_to(DrawingProgramCache::CACHE_NODE_RESOLUTION);} catch(...) {}
    try{j.at("debug").at("maxCacheNodes").get_to(DrawingProgramCache::MAXIMUM_DRAW_CACHE_SURFACES);} catch(...) {}
    try{j.at("debug").at("maxComponentsInNode").get_to(DrawingProgramCache::MAXIMUM_COMPONENTS_IN_SINGLE_NODE);} catch(...) {}
    try{j.at("debug").at("componentCountToForceCacheRebuild").get_to(DrawingProgramCache::MINIMUM_COMPONENTS_TO_START_REBUILD);} catch(...) {}
    try{j.at("debug").at("maximumFrameTimeToForceCacheRebuild").get_to(DrawingProgramCache::MILLISECOND_FRAME_TIME_TO_FORCE_CACHE_REFRESH);} catch(...) {}
    try{j.at("debug").at("millisecondMinimumTimeToCheckForCacheRebuild").get_to(DrawingProgramCache::MILLISECOND_MINIMUM_TIME_TO_CHECK_FORCE_REFRESH);} catch(...) {}
}
