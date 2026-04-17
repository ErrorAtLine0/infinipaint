#include "MainProgram.hpp"
#include "CustomEvents.hpp"
#include "Screens/FileSelectScreen.hpp"
#include "VersionConstants.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
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
#include "Screens/DrawingProgramScreen.hpp"
#include "Screens/FileSelectScreen.hpp"

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

MainProgram::MainProgram():
    input(*this),
    fonts(std::make_shared<FontData>()),
    g(*this)
{
    screen = std::make_unique<DrawingProgramScreen>(*this);
    Logger::get().add_log("WORLDFATAL", [&](const std::string& text) {
        *logFile << "[WORLDFATAL] " << text << std::endl;
        std::cout << "[WORLDFATAL] " << text << std::endl;
        logMessages.emplace_front(UserLogMessage{text, UserLogMessage::COLOR_ERROR});
        if(logMessages.size() == LOG_SIZE)
            logMessages.pop_back();
        g.gui.set_to_layout();
    });

    Logger::get().add_log("USERINFO", [&](const std::string& text) {
        *logFile << "[USERINFO] " << text << std::endl;
        std::cout << "[USERINFO] " << text << std::endl;
        logMessages.emplace_front(UserLogMessage{text, UserLogMessage::COLOR_NORMAL});
        if(logMessages.size() == LOG_SIZE)
            logMessages.pop_back();
        g.gui.set_to_layout();
    });

    Logger::get().add_log("CHAT", [&](const std::string& text) {
        *logFile << "[CHAT] " << text << std::endl;
        std::cout << "[CHAT] " << text << std::endl;
    });
}

void MainProgram::update() {
    deltaTime.update_time_since();
    deltaTime.update_time_point();
    input.update();
    g.update();
    screen->update();
    for(auto& w : worlds)
        w->update();
    NetLibrary::update();
    post_callback();
}

bool MainProgram::app_close_requested() {
    return screen->app_close_requested();
}

void MainProgram::update_display_names() {
    for(auto& w : worlds) {
        if(!w->netObjMan.is_connected())
            w->ownClientData->set_display_name(conf.displayName);
    }
}

void MainProgram::init_net_library() {
    NetLibrary::init(conf.configPath / "p2p.json");
}

void MainProgram::save_config() {
    std::ofstream f(conf.configPath / "config.json");
    if(f.is_open()) {
        using json = nlohmann::json;
        json j;

        j["version"] = VersionConstants::CURRENT_VERSION_STRING;
        j["settings"] = conf.get_config_json(input);
        j["window"]["pos"] = window.writtenPos;
        j["window"]["size"] = window.writtenSize;
        j["window"]["maximized"] = window.maximized;
        j["window"]["fullscreen"] = window.fullscreen;
        j["fileselectorpath"] = conf.currentSearchPath;
        j["toolConfig"] = toolConfig;

        f << std::setw(4) << j;
        f.close();
    }
    conf.save_palettes();
}

void MainProgram::load_config() {
    std::ifstream f(conf.configPath / "config.json");
    conf.currentSearchPath = homePath;
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
                j.at("fileselectorpath").get_to(conf.currentSearchPath);
            }
            catch(...) {
                conf.currentSearchPath = homePath;
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
        conf.viewWebVersionWelcome = true;
#endif
    conf.load_palettes();
    g.load_theme(conf.configPath, conf.themeCurrentlyLoaded);
    conf.load_licenses();

    NetLibrary::copy_default_p2p_config_to_path(conf.configPath / "p2p.json");

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
        if(conf.antialiasing == GlobalConfig::AntiAliasing::DYNAMIC_MSAA)
            window.intermediateSurfaceMSAA = create_native_surface(window.size, true);
        else
            window.intermediateSurfaceMSAA = nullptr;

        g.delete_cache_surface();

        DrawingProgramCache::delete_all_draw_cache();
    }
}

void MainProgram::draw_world(SkCanvas* canvas, std::shared_ptr<World> worldToDraw, const DrawData& drawData) {
    canvas->clear(drawData.transparentBackground ? SkColor4f{0.0f, 0.0f, 0.0f, 0.0f} : worldToDraw->canvasTheme.get_back_color());
    DrawData drawDataCopy = drawData;
    drawDataCopy.skiaAA = conf.antialiasing == GlobalConfig::AntiAliasing::SKIA;
    worldToDraw->draw(canvas, drawDataCopy);
}

void MainProgram::draw(SkCanvas* canvas) {
    screen->draw(canvas);
    g.draw(canvas, conf.antialiasing == GlobalConfig::AntiAliasing::SKIA);
}

sk_sp<SkSurface> MainProgram::create_native_surface(Vector2i resolution, bool isMSAA) {
    if(window.canCreateSurfaces) {
        SkImageInfo imgInfo = SkImageInfo::MakeN32Premul(resolution.x(), resolution.y());
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

void MainProgram::create_new_tab(const CustomEvents::OpenInfiniPaintFileEvent& openFile) {
    std::shared_ptr<World> newWorld;
    try {
        newWorld = std::make_shared<World>(*this, openFile);
    }
    catch(const std::runtime_error& e) {
        Logger::get().log("WORLDFATAL", "Failed to open canvas: " + (openFile.filePathSource.has_value() ? openFile.filePathSource.value().string() : "NO PATH") + " with error: " + e.what());
        return;
    }
    worlds.emplace_back(newWorld);
    switch_to_tab(worlds.size() - 1);
    g.gui.set_to_layout();
}

void MainProgram::set_tab_to_close(World* world) {
    tabsToClose.emplace(world);
}

void MainProgram::switch_to_tab(size_t wIndex) {
    world = worlds[wIndex];
    worldIndex = wIndex;
}

void MainProgram::post_callback() {
    close_set_to_close_tabs();
    g.post_callback();
}

void MainProgram::input_add_file_to_canvas_callback(const CustomEvents::AddFileToCanvasEvent& addFile) {
    screen->input_add_file_to_canvas_callback(addFile);
    post_callback();
}

void MainProgram::input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile) {
    screen->input_open_infinipaint_file_callback(openFile);
    post_callback();
}

void MainProgram::input_paste_callback(const CustomEvents::PasteEvent& paste) {
    g.input_paste_callback(paste);
    screen->input_paste_callback(paste);
    post_callback();
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
        post_callback();
        return true;
    }
    return false;
}

void MainProgram::input_drop_file_callback(const InputManager::DropCallbackArgs& drop) {
    screen->input_drop_file_callback(drop);
    post_callback();
}

void MainProgram::input_drop_text_callback(const InputManager::DropCallbackArgs& drop) {
    screen->input_drop_text_callback(drop);
    post_callback();
}

void MainProgram::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    if(key.key == InputManager::KEY_FULLSCREEN) {
        if(key.down && !key.repeat)
            toggle_full_screen();
    }
    g.input_key_callback(key);
    screen->input_key_callback(key);
    post_callback();
}

void MainProgram::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {
    g.input_text_key_callback(key);
    screen->input_text_key_callback(key);
    post_callback();
}

void MainProgram::input_text_callback(const InputManager::TextCallbackArgs& text) {
    g.input_text_callback(text);
    screen->input_text_callback(text);
    post_callback();
}

void MainProgram::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    g.input_mouse_button_callback(button);
    screen->input_mouse_button_callback(button);
    post_callback();
}

void MainProgram::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    g.input_mouse_motion_callback(motion);
    screen->input_mouse_motion_callback(motion);
    post_callback();
}

void MainProgram::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) {
    g.input_mouse_wheel_callback(wheel);
    screen->input_mouse_wheel_callback(wheel);
    post_callback();
}

void MainProgram::input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button) {
    screen->input_pen_button_callback(button);
    post_callback();
}

void MainProgram::input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch) {
    screen->input_pen_touch_callback(touch);
    post_callback();
}

void MainProgram::input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion) {
    screen->input_pen_motion_callback(motion);
    post_callback();
}

void MainProgram::input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) {
    screen->input_pen_axis_callback(axis);
    post_callback();
}

void MainProgram::input_multi_finger_touch_callback(const InputManager::MultiFingerTouchCallbackArgs& touch) {
    screen->input_multi_finger_touch_callback(touch);
    post_callback();
}

void MainProgram::input_multi_finger_motion_callback(const InputManager::MultiFingerMotionCallbackArgs& motion) {
    screen->input_multi_finger_motion_callback(motion);
    post_callback();
}

void MainProgram::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
    screen->input_finger_touch_callback(touch);
    post_callback();
}

void MainProgram::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {
    screen->input_finger_motion_callback(motion);
    post_callback();
}

void MainProgram::input_window_resize_callback(const InputManager::WindowResizeCallbackArgs& w) {
    for(auto& wor : worlds)
        wor->drawData.cam.set_viewing_area(window.size.cast<float>());
    g.window_update();
    screen->input_window_resize_callback(w);
    post_callback();
}

void MainProgram::input_window_scale_callback(const InputManager::WindowScaleCallbackArgs& w) {
    g.window_update();
    screen->input_window_scale_callback(w);
    post_callback();
}

void MainProgram::toggle_full_screen() {
    window.fullscreen = !window.fullscreen;
    SDL_SetWindowFullscreen(window.sdlWindow, window.fullscreen);
}

void MainProgram::close_set_to_close_tabs() {
    if(!tabsToClose.empty()) {
        std::erase_if(worlds, [&](auto& w) {
            if(tabsToClose.contains(w.get())) {
                if(w == world)
                    world = nullptr;
                return true;
            }
            return false;
        });
        if(worlds.empty())
            create_new_tab({
                .isClient = false
            });
        else if(world)
            worldIndex = std::find(worlds.begin(), worlds.end(), world) - worlds.begin();
        else
            switch_to_tab(0);
        tabsToClose.clear();
        g.gui.set_to_layout();
    }
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
