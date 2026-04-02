#include "MainProgram.hpp"
#include "VersionConstants.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <chrono>
#include <include/core/SkAlphaType.h>
#include <include/core/SkColor.h>
#include <include/core/SkPaint.h>
#include <filesystem>
#include <iostream>
#include <include/core/SkFont.h>
#include <include/core/SkData.h>
#include <include/core/SkSurfaceProps.h>
#include "InputManager.hpp"
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>
#include <Eigen/Core>
#include <fstream>
#include <cereal/archives/json.hpp>
#include <Helpers/Serializers.hpp>
#include <nlohmann/json.hpp>

#include <include/core/SkSurface.h>
#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include <include/gpu/graphite/Surface.h>
#elif USE_SKIA_BACKEND_GANESH
    #include <include/gpu/ganesh/GrDirectContext.h>
    #include <include/gpu/ganesh/SkSurfaceGanesh.h>
#endif

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

#include <Helpers/Logger.hpp>
#include "DrawCollision.hpp"

MainProgram::MainProgram():
    input(*this),
    fonts(std::make_shared<FontData>()),
    toolbar(*this),
    g(*this)
{
    Logger::get().add_log("WORLDFATAL", [&](const std::string& text) {
        *logFile << "[WORLDFATAL] " << text << std::endl;
        std::cout << "[WORLDFATAL] " << text << std::endl;
        logMessages.emplace_front(Toolbar::LogMessage{text, Toolbar::LogMessage::COLOR_ERROR});
        if(logMessages.size() == LOG_SIZE)
            logMessages.pop_back();
    });

    Logger::get().add_log("USERINFO", [&](const std::string& text) {
        *logFile << "[USERINFO] " << text << std::endl;
        std::cout << "[USERINFO] " << text << std::endl;
        logMessages.emplace_front(Toolbar::LogMessage{text, Toolbar::LogMessage::COLOR_NORMAL});
        if(logMessages.size() == LOG_SIZE)
            logMessages.pop_back();
    });

    Logger::get().add_log("CHAT", [&](const std::string& text) {
        *logFile << "[CHAT] " << text << std::endl;
        std::cout << "[CHAT] " << text << std::endl;
    });
}

void MainProgram::update() {
    while(std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - lastFrameTime).count() < 1.0 / conf.fpsLimit);
    lastFrameTime = std::chrono::steady_clock::now();

    input.update();

    for(auto& s : setTabsToClose)
        std::erase(worlds, s.lock());

    std::erase_if(worlds, [&](const std::shared_ptr<World>& w) {
        return w->setToDestroy;
    });

    if(worldIndex >= worlds.size())
        worldIndex = worlds.size() - 1;
    if(worlds.size() == 0)
        new_tab({
            .isClient = false
        }, true);

    std::shared_ptr<World> oldWorld = world;
    world = worlds[worldIndex];
    if(oldWorld != world) {
        if(oldWorld)
            oldWorld->on_tab_out();
    }

    deltaTime.update_time_since();
    deltaTime.update_time_point();

    for(auto& w : worlds) {
        if(w == world)
            w->focus_update();
        else
            w->unfocus_update();
    }

    g.update();

    NetLibrary::update();

    if(tabSetToOpen) {
        tabSetToOpen = false;
        new_tab_open();
    }

    return;
}

bool MainProgram::app_close_requested() {
    return toolbar.app_close_requested();
}

void MainProgram::update_display_names() {
    for(auto& w : worlds) {
        if(!w->netObjMan.is_connected())
            w->ownClientData->set_display_name(conf.displayName);
    }
}

void MainProgram::new_tab(const World::OpenWorldInfo& tabInfo, bool createSameThread) {
    newTabToOpenInfo = tabInfo;
    if(createSameThread)
        new_tab_open();
    else
        tabSetToOpen = true;
}

void MainProgram::new_tab_open() {
    std::unique_ptr<World> newWorld;
    try {
        newWorld = std::make_unique<World>(*this, newTabToOpenInfo);
    }
    catch(const std::runtime_error& e) {
        Logger::get().log("WORLDFATAL", "Failed to open canvas: " + (newTabToOpenInfo.filePathSource.has_value() ? newTabToOpenInfo.filePathSource.value().string() : "NO PATH") + " with error: " + e.what());
        return;
    }
    worlds.emplace_back(std::move(newWorld));
    worldIndex = worlds.size() - 1;
}

void MainProgram::set_tab_to_close(const std::weak_ptr<World>& tabToClose) {
    setTabsToClose.emplace_back(tabToClose);
}

void MainProgram::init_net_library() {
    NetLibrary::init(configPath / "p2p.json");
}

void MainProgram::save_config() {
    std::ofstream f(configPath / "config.json");
    if(f.is_open()) {
        using json = nlohmann::json;
        json j;

        j["version"] = VersionConstants::CURRENT_VERSION_STRING;
        j["settings"] = conf.get_config_json(input);
        j["window"]["pos"] = window.writtenPos;
        j["window"]["size"] = window.writtenSize;
        j["window"]["maximized"] = window.maximized;
        j["window"]["fullscreen"] = window.fullscreen;
        j["fileselectorpath"] = toolbar.file_selector_path();
        j["toolConfig"] = toolConfig;

        f << std::setw(4) << j;
        f.close();
    }
    toolbar.save_palettes();

#ifdef __EMSCRIPTEN__
    EM_ASM(
        FS.syncfs(false, (err) => {
            if(err)
                console.log("Error syncing IDBFS: ", err);
            else
                console.log("Synced to IDBFS");
        });
    );
#endif
}

void MainProgram::load_config() {
    std::ifstream f(configPath / "config.json");
    toolbar.file_selector_path() = homePath;
    if(f.is_open()) {
        using json = nlohmann::json;

        try {
            json j;
            f >> j;

            VersionNumber version(0, 0, 1);
            try {
                std::string versionStr;
                j.at("version").get_to(versionStr);
                auto optVersion = version_str_to_version_numbers(versionStr);
                if(optVersion.has_value())
                    version = optVersion.value();
            }
            catch(...) {}

            try {
                conf.set_config_json(input, j["settings"], version);
            }
            catch(...) {}
#ifndef __EMSCRIPTEN__
            try {
                j.at("window").at("size").get_to(window.size);
                window.writtenSize = window.size;
                j.at("window").at("pos").get_to(window.pos);
                window.writtenPos = window.pos;
                j.at("window").at("maximized").get_to(window.maximized);
                j.at("window").at("fullscreen").get_to(window.fullscreen);
            }
            catch(...) {}
            try {
                j.at("fileselectorpath").get_to(toolbar.file_selector_path());
            }
            catch(...) {
                toolbar.file_selector_path() = homePath;
            }
#endif
            try {
                j.at("toolConfig").get_to(toolConfig);
            }
            catch(...) {}
            f.close();
        } catch(...) {}
    }
#ifdef __EMSCRIPTEN__
    else
        toolbar.viewWebVersionWelcome = true;
#endif
    toolbar.load_palettes();
    g.load_theme(configPath, conf.themeCurrentlyLoaded);
    toolbar.load_licenses();

    NetLibrary::copy_default_p2p_config_to_path(configPath / "p2p.json");

    update_display_names();
    set_vsync_value(conf.vsyncValue);
    refresh_draw_surfaces();
}

void MainProgram::set_vsync_value(int vsyncValue) {
    if(!SDL_GL_SetSwapInterval(vsyncValue)) {
        Logger::get().log("INFO", "Vsync value " + std::to_string(vsyncValue) + " not available. Setting to 1");
        if(vsyncValue == -1) {
            Logger::get().log("USERINFO", "Adaptive VSync not available. Setting Vsync to On");
            vsyncValue = 1;
        }
        SDL_GL_SetSwapInterval(1);
    }
    conf.vsyncValue = vsyncValue;
}

void MainProgram::update_scale_and_density() {
#ifdef __EMSCRIPTEN__
    window.scale = 1.0f;
#else
    window.scale = conf.applyDisplayScale ? SDL_GetWindowDisplayScale(window.sdlWindow) : 1.0f;
#endif
    window.density = SDL_GetWindowPixelDensity(window.sdlWindow);
}

float MainProgram::get_scale_and_density_factor_gui() {
    return window.scale * window.density;
}

void MainProgram::refresh_draw_surfaces() {
    if(window.canCreateSurfaces) {
        window.defaultMSAASampleCount = 0;
        window.defaultMSAASurfaceProps = SkSurfaceProps(conf.antialiasing == GlobalConfig::AntiAliasing::DYNAMIC_MSAA ? SkSurfaceProps::kDynamicMSAA_Flag : SkSurfaceProps::kDefault_Flag, kUnknown_SkPixelGeometry);
        window.intermediateSurface = create_native_surface(window.size, true);
        window.intermediateCanvas = window.intermediateSurface->getCanvas();

        if(!window.intermediateCanvas)
            throw std::runtime_error("[refresh_draw_surfaces] Could not create intermediate canvas");

        DrawingProgramCache::delete_all_draw_cache();
    }
}

void MainProgram::draw(SkCanvas* canvas, std::shared_ptr<World> worldToDraw, const DrawData& drawData) {
    canvas->clear(drawData.transparentBackground ? SkColor4f{0.0f, 0.0f, 0.0f, 0.0f} : worldToDraw->canvasTheme.get_back_color());
    DrawData drawDataCopy = drawData;
    drawDataCopy.skiaAA = conf.antialiasing == GlobalConfig::AntiAliasing::SKIA;
    worldToDraw->draw(canvas, drawDataCopy);
    if(drawGui)
        g.draw(canvas, drawDataCopy.skiaAA);
}

sk_sp<SkSurface> MainProgram::create_native_surface(Vector2i resolution, bool isMSAA) {
    if(window.canCreateSurfaces) {
        #ifdef USE_BACKEND_OPENGLES_3_0
            SkImageInfo imgInfo = SkImageInfo::Make(resolution.x(), resolution.y(), kRGBA_8888_SkColorType, kPremul_SkAlphaType);
        #else
            SkImageInfo imgInfo = SkImageInfo::MakeN32Premul(resolution.x(), resolution.y());
        #endif
        SkSurfaceProps defaultProps;
        #ifdef USE_SKIA_BACKEND_GRAPHITE
            auto surfaceToRet = SkSurfaces::RenderTarget(window.recorder(), imgInfo, skgpu::Mipmapped::kNo, isMSAA ? &window.defaultMSAASurfaceProps : &defaultProps);
        #elif USE_SKIA_BACKEND_GANESH
            auto surfaceToRet = SkSurfaces::RenderTarget(window.ctx.get(), skgpu::Budgeted::kNo, imgInfo, isMSAA ? window.defaultMSAASampleCount : 0, isMSAA ? &window.defaultMSAASurfaceProps : &defaultProps);
        #endif

        if(!surfaceToRet)
            throw std::runtime_error("[MainProgram::create_native_surface] Could not make native surface");

        return surfaceToRet;
    }
    return nullptr;
}

bool MainProgram::input_keybind_callback(const Vector2ui32& newKey) {
    if(keybindWaiting.has_value()) {
        unsigned v = keybindWaiting.value();

        input.keyAssignments.erase(newKey);
        auto f = std::find_if(input.keyAssignments.begin(), input.keyAssignments.end(), [&](auto& p) {
            return p.second == v;
        });
        if(f != input.keyAssignments.end())
            input.keyAssignments.erase(f);
        input.keyAssignments.emplace(newKey, v);
        keybindWaiting = std::nullopt;
        g.gui.set_to_layout();
        return true;
    }
    return false;
}

void MainProgram::input_drop_file_callback(const InputManager::DropCallbackArgs& drop) {
    if(std::filesystem::is_regular_file(drop.data)) {
        std::filesystem::path droppedFilePath(drop.data);
        if(droppedFilePath.has_extension() && droppedFilePath.extension().string() == std::string("." + World::FILE_EXTENSION)) {
            new_tab({
                .isClient = false,
                .filePathSource = droppedFilePath
            }, true);
            return;
        }
    }
    if(world)
        world->input_drop_file_callback(drop);
}

void MainProgram::input_drop_text_callback(const InputManager::DropCallbackArgs& drop) {
    if(world)
        world->input_drop_text_callback(drop);
}

void MainProgram::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    switch(key.key) {
        case InputManager::KEY_NOGUI: {
            if(key.down && !key.repeat)
                drawGui = !drawGui;
            break;
        }
        case InputManager::KEY_FULLSCREEN: {
            if(key.down && !key.repeat)
                input.toggleFullscreen = true;
        }
    }
    g.input_key_callback(key);
    if(world)
        world->input_key_callback(key);
}

void MainProgram::input_text_key_callback() {
    g.input_text_key_callback();
}

void MainProgram::input_text_input_callback() {
    g.input_text_input_callback();
}

void MainProgram::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    g.input_mouse_button_callback(button);
    if(world)
        world->input_mouse_button_callback(button);
}

void MainProgram::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    g.input_mouse_motion_callback(motion);
    if(world)
        world->input_mouse_motion_callback(motion);
}

void MainProgram::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) {
    g.input_mouse_wheel_callback(wheel);
    if(world)
        world->input_mouse_wheel_callback(wheel);
}

void MainProgram::input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button) {
    if(world)
        world->input_pen_button_callback(button);
}

void MainProgram::input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch) {
    if(world)
        world->input_pen_touch_callback(touch);
}

void MainProgram::input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion) {
    if(world)
        world->input_pen_motion_callback(motion);
}

void MainProgram::input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) {
    if(world)
        world->input_pen_axis_callback(axis);
}

void MainProgram::input_multi_finger_touch_callback(const InputManager::MultiFingerTouchCallbackArgs& touch) {
    if(world)
        world->input_multi_finger_touch_callback(touch);
}

void MainProgram::input_multi_finger_motion_callback(const InputManager::MultiFingerMotionCallbackArgs& motion) {
    if(world)
        world->input_multi_finger_motion_callback(motion);
}

void MainProgram::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
}

void MainProgram::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {
}

bool MainProgram::network_being_used() {
    for(auto& w : worlds) {
        if(w->netObjMan.is_connected())
            return true;
    }
    return false;
}

bool MainProgram::net_server_hosted() {
    for(auto& w : worlds) {
        if(w->netServer)
            return true;
    }
    return false;
}

void MainProgram::early_destroy() {
    for(auto& w : worlds)
        w->early_destroy();
}

MainProgram::~MainProgram() {
    NetLibrary::destroy();
}
