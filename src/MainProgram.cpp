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

#include "MainProgram.hpp"
#include "CustomEvents.hpp"
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
#include "NetThreadManager.hpp"
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>
#include <Eigen/Core>
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

#define UPDATE_NOTIFICATION_URL "https://infinipaint.com/updateNotificationVersion.txt"

MainProgram::MainProgram():
    input(*this),
    fonts(std::make_shared<FontData>()),
    g(*this)
{
    g.gui.io.layoutRun = [&] {
        screen->gui_layout_run();
    };

    Logger::get().set_log_function(Logger::LogType::WORLDFATAL, [&](const std::string& text) {
        *logFile << "[WORLDFATAL] " << text << std::endl;
        std::cout << "[WORLDFATAL] " << text << std::endl;
        logMessages.emplace_front(UserLogMessage{text, UserLogMessage::COLOR_ERROR, UserLogMessage::DISPLAY_DESKTOP_ONLY});
        if(logMessages.size() == LOG_SIZE)
            logMessages.pop_back();
        g.gui.set_to_layout();
    });

    Logger::get().set_log_function(Logger::LogType::USERINFO, [&](const std::string& text) {
        *logFile << "[USERINFO] " << text << std::endl;
        std::cout << "[USERINFO] " << text << std::endl;
        logMessages.emplace_front(UserLogMessage{text, UserLogMessage::COLOR_NORMAL, UserLogMessage::DISPLAY_FOR_ALL});
        if(logMessages.size() == LOG_SIZE)
            logMessages.pop_back();
        g.gui.set_to_layout();
    });

    Logger::get().set_log_function(Logger::LogType::DESKTOP_USERINFO, [&](const std::string& text) {
        *logFile << "[DESKTOP_USERINFO] " << text << std::endl;
        std::cout << "[DESKTOP_USERINFO] " << text << std::endl;
        logMessages.emplace_front(UserLogMessage{text, UserLogMessage::COLOR_NORMAL, UserLogMessage::DISPLAY_DESKTOP_ONLY});
        if(logMessages.size() == LOG_SIZE)
            logMessages.pop_back();
        g.gui.set_to_layout();
    });

    Logger::get().set_log_function(Logger::LogType::PHONE_USERINFO, [&](const std::string& text) {
        *logFile << "[PHONE_USERINFO] " << text << std::endl;
        std::cout << "[PHONE_USERINFO] " << text << std::endl;
        logMessages.emplace_front(UserLogMessage{text, UserLogMessage::COLOR_NORMAL, UserLogMessage::DISPLAY_PHONE_ONLY});
        if(logMessages.size() == LOG_SIZE)
            logMessages.pop_back();
        g.gui.set_to_layout();
    });

    Logger::get().set_log_function(Logger::LogType::CHAT, [&](const std::string& text) {
        *logFile << "[CHAT] " << text << std::endl;
        std::cout << "[CHAT] " << text << std::endl;
    });
}

void MainProgram::update() {
    deltaTime.update_time_since();
    deltaTime.update_time_point();
    input.update();
    g.update();
    update_notification_check();
    screen->update();
    background_update();
    NetThreadManager::get().synchronous_update();
    post_callback();
}

void MainProgram::update_notification_check() {
#ifndef __EMSCRIPTEN__
    if(!updateCheckerData.updateCheckDone) {
        if(conf.checkForUpdates) {
            if(!updateCheckerData.versionFile)
                updateCheckerData.versionFile = FileDownloader::download_data_from_url(UPDATE_NOTIFICATION_URL);
            else {
                switch(updateCheckerData.versionFile->status) {
                    case FileDownloader::DownloadData::Status::IN_PROGRESS:
                        break;
                    case FileDownloader::DownloadData::Status::SUCCESS: {
                        updateCheckerData.updateCheckDone = true;
                        std::optional<VersionNumber> newVersion = version_str_to_version_numbers(updateCheckerData.versionFile->str);
                        std::optional<VersionNumber> currentVersion = VersionConstants::CURRENT_VERSION_NUMBER;
                        if(newVersion.has_value() && currentVersion.has_value()) {
                            VersionNumber& newV = newVersion.value();
                            VersionNumber& currentV = currentVersion.value();
                            updateCheckerData.newVersionStr = version_numbers_to_version_str(newV);
                            Logger::get().log(Logger::LogType::INFO, "Latest online version is v" + updateCheckerData.newVersionStr);
                            if(newV > currentV) {
                                updateCheckerData.showGui = true;
                                Logger::get().log(Logger::LogType::PHONE_USERINFO, "New version v" + updateCheckerData.newVersionStr + " available");
                                g.gui.set_to_layout();
                            }
                            else if(newV == currentV)
                                Logger::get().log(Logger::LogType::INFO, "Current version is up to date");
                            else
                                Logger::get().log(Logger::LogType::INFO, "Local version has larger version number than the latest online one");
                        }
                        else
                            Logger::get().log(Logger::LogType::INFO, "Update notification file couldn't be converted to version numbers");
                        updateCheckerData.versionFile = nullptr;
                        break;
                    }
                    case FileDownloader::DownloadData::Status::FAILURE:
                        Logger::get().log(Logger::LogType::INFO, "Failed to check for updates");
                        updateCheckerData.updateCheckDone = true;
                        updateCheckerData.versionFile = nullptr;
                        break;
                }
            }
        }
        else
            updateCheckerData.updateCheckDone = true;
    }
#endif
}

void MainProgram::background_update() {
    bool isAnyWorldConnected = false;
    for(auto& w : worlds) {
        w->update();
        isAnyWorldConnected |= ((w->netServer && !w->netServer->is_disconnected()) || (w->netClient && !w->netClient->is_disconnected()));
    }
    if(!isAnyWorldConnected)
        NetThreadManager::get().destroy();
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
    NetThreadManager::get().init(this);
}

void MainProgram::save_config() {
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

    std::stringstream f;
    f << std::setw(4) << j;
    SDL_SaveFile((conf.configPath / "config.json").string().c_str(), f.view().data(), f.view().size());

    conf.save_palettes();
}

void MainProgram::load_config() {
    conf.currentSearchPath = homePath;
    using json = nlohmann::json;

    try {
        json j(nlohmann::json::parse(read_file_to_string(conf.configPath / "config.json")));

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
    } catch(...) {}
    conf.load_palettes();
    g.load_theme(conf.configPath, conf.themeCurrentlyLoaded);
    conf.load_licenses();

    NetLibrary::copy_default_p2p_config_to_path(conf.configPath / "p2p.json");
    NetLibrary::init_config(conf.configPath / "p2p.json");

    update_display_names();
    set_vsync_value(conf.vsyncValue);
    refresh_draw_surfaces();
}

void MainProgram::set_vsync_value(int vsyncValue) {
    if(!SDL_GL_SetSwapInterval(vsyncValue)) {
        Logger::get().log(Logger::LogType::INFO, "Vsync value " + std::to_string(vsyncValue) + " not available. Setting to 1");
        if(vsyncValue == -1) {
            Logger::get().log(Logger::LogType::DESKTOP_USERINFO, "Adaptive VSync not available. Setting Vsync to On");
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
        Logger::get().log(Logger::LogType::WORLDFATAL, "Failed to open canvas: " + (openFile.filePathSource.has_value() ? openFile.filePathSource.value().string() : "NO PATH") + " with error: " + e.what());
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
    g.gui.run_post_callback_func();
    close_set_to_close_tabs();
    run_new_screen_func();
    g.gui.layout_if_necessary();
}

void MainProgram::input_app_about_to_go_to_background_callback() {
    screen->input_app_about_to_go_to_background_callback();
    save_config();
    post_callback();
    NetThreadManager::get().go_to_background();
}

void MainProgram::input_app_about_to_go_to_foreground_callback() {
    NetThreadManager::get().go_to_foreground();
    screen->input_app_about_to_go_to_foreground_callback();
    post_callback();
}

void MainProgram::input_add_file_to_canvas_callback(const CustomEvents::AddFileToCanvasEvent& addFile) {
    screen->input_add_file_to_canvas_callback(addFile);
    post_callback();
}

void MainProgram::input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile) {
    screen->input_open_infinipaint_file_callback(openFile);
    g.gui.set_to_layout();
    post_callback();
}

void MainProgram::input_paste_callback(const CustomEvents::PasteEvent& paste) {
    g.input_paste_callback(paste);
    screen->input_paste_callback(paste);
    post_callback();
}

void MainProgram::input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) {
    g.input_android_text_box_input_callback(textboxInput);
    screen->input_android_text_box_input_callback(textboxInput);
    post_callback();
}

void MainProgram::input_global_back_button_callback() {
    screen->input_global_back_button_callback();
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
    g.input_finger_touch_callback(touch);
    screen->input_finger_touch_callback(touch);
    post_callback();
}

void MainProgram::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {
    g.input_finger_motion_callback(motion);
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

std::optional<InputManager::TextBoxStartInfo> MainProgram::get_text_box_start_info() {
    auto toRet = g.get_text_box_start_info();
    if(toRet)
        return toRet;
    return screen->get_text_box_start_info();
}

void MainProgram::toggle_full_screen() {
    window.fullscreen = !window.fullscreen;
    SDL_SetWindowFullscreen(window.sdlWindow, window.fullscreen);
}

void MainProgram::set_first_screen(std::unique_ptr<Screen> firstScreen) {
    set_screen([&](std::unique_ptr<Screen>){ return std::move(firstScreen); });
    run_new_screen_func();
}

void MainProgram::run_new_screen_func() {
    if(newScreenFunc) {
        screen = newScreenFunc(std::move(screen));
        g.gui.io.redrawSurface = true;
        g.gui.set_to_layout();
        newScreenFunc = nullptr;
    }
}

float MainProgram::calculate_gui_scale() {
    return screen->calculate_gui_scale();
}

void MainProgram::set_screen(std::function<std::unique_ptr<Screen>(std::unique_ptr<Screen>)> screenFunc) {
    newScreenFunc = screenFunc;
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
        tabsToClose.clear();
        screen->on_tab_close();
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
    screen = nullptr; // Destroy screen first to let destructor run
    NetThreadManager::get().destroy();
}
