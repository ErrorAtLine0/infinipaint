#include "MainProgram.hpp"
#include "DrawingProgram/DrawingProgramToolBase.hpp"
#include "Helpers/NetworkingObjects/NetObjManagerTypeList.hpp"
#include "VersionConstants.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/details/helpers.hpp"
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
#include "Server/CommandList.hpp"
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
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include <Helpers/NetworkingObjects/NetObjGenericSerializedClass.hpp>

MainProgram::MainProgram():
    fonts(std::make_shared<FontData>()),
    toolbar(*this)
{
    netObjectTypeList = std::make_shared<NetworkingObjects::NetObjManagerTypeList>();
    NetworkingObjects::register_generic_serialized_class<std::string>(*netObjectTypeList);
    NetworkingObjects::register_ordered_list_class<std::string>(*netObjectTypeList);

    displayName = Random::get().alphanumeric_str(10);

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
    while(std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - lastFrameTime).count() < 1.0 / fpsLimit);
    lastFrameTime = std::chrono::steady_clock::now();

    std::erase_if(input.droppedItems, [&](auto& droppedItem) {
        if(droppedItem.dataPath.has_value() && std::filesystem::is_regular_file(droppedItem.dataPath.value())) {
            const std::filesystem::path& droppedFilePath = droppedItem.dataPath.value();
            if(droppedFilePath.has_extension() && droppedFilePath.extension().string() == std::string("." + World::FILE_EXTENSION)) {
                new_tab({
                    .conType = World::CONNECTIONTYPE_LOCAL,
                    .filePathSource = droppedFilePath
                }, true);
                return true;
            }
        }
        return false;
    });

    if(setTabToClose) {
        worlds.erase(worlds.begin() + setTabToClose.value());
        setTabToClose.reset();
    }

    std::erase_if(worlds, [&](const std::shared_ptr<World>& w) {
        return w->setToDestroy;
    });

    if(worldIndex >= worlds.size())
        worldIndex = worlds.size() - 1;
    if(worlds.size() == 0)
        new_tab({
            .conType = World::CONNECTIONTYPE_LOCAL
        }, true);

    std::shared_ptr<World> oldWorld = world;
    world = worlds[worldIndex];

    deltaTime.update_time_since();
    deltaTime.update_time_point();

    toolbar.initialize_io_before_update();

    for(auto& w : worlds) {
        if(w == world)
            w->focus_update();
        else
            w->unfocus_update();
    }

    toolbar.update(); // GUI should be setup after the world data has been fully updated for this frame, so that the GUI reflects the current state of the world data

    if(input.key(InputManager::KEY_NOGUI).pressed)
        drawGui = !drawGui;

    if(input.key(InputManager::KEY_FULLSCREEN).pressed)
        input.toggleFullscreen = true;

    NetLibrary::update();

    if(tabSetToOpen) {
        tabSetToOpen = false;
        new_tab_open();
    }

    return;
}

void MainProgram::update_display_names() {
    for(auto& w : worlds) {
        if(!w->network_being_used())
            w->displayName = displayName;
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

void MainProgram::set_tab_to_close(size_t tabToClose) {
    setTabToClose = tabToClose;
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
        j["settings"] = toolbar.get_config_json();
        j["window"]["pos"] = window.writtenPos;
        j["window"]["size"] = window.writtenSize;
        j["window"]["maximized"] = window.maximized;
        j["window"]["fullscreen"] = window.fullscreen;
        j["fileselectorpath"] = toolbar.file_selector_path();

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
                toolbar.set_config_json(j["settings"], version);
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
            f.close();
        } catch(...) {}
    }
#ifdef __EMSCRIPTEN__
    else
        toolbar.viewWebVersionWelcome = true;
#endif
    toolbar.load_palettes();
    toolbar.load_theme();
    toolbar.load_licenses();

    NetLibrary::copy_default_p2p_config_to_path(configPath / "p2p.json");
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
    window.vsyncValue = vsyncValue;
}

void MainProgram::update_scale_and_density() {
#ifdef __EMSCRIPTEN__
    window.scale = 1.0f;
#else
    window.scale = window.applyDisplayScale ? SDL_GetWindowDisplayScale(window.sdlWindow) : 1.0f;
#endif
    window.density = SDL_GetWindowPixelDensity(window.sdlWindow);
}

float MainProgram::get_scale_and_density_factor_gui() {
    return window.scale * window.density;
}

void MainProgram::draw(SkCanvas* canvas) {
    canvas->clear(transparentBackground ? SkColor4f{0.0f, 0.0f, 0.0f, 0.0f} : world->canvasTheme.backColor);
    world->draw(canvas);
    toolbar.draw(canvas);
}

bool MainProgram::network_being_used() {
    for(auto& w : worlds) {
        if(w->network_being_used())
            return true;
    }
    return false;
}

bool MainProgram::net_server_hosted() {
    for(auto& w : worlds) {
        if(w->conType == World::CONNECTIONTYPE_SERVER)
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
