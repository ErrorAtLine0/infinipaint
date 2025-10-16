#include "MainProgram.hpp"
#include "CollabTextBox/CollabTextBoxCursor.hpp"
#include "DrawingProgram/DrawingProgramToolBase.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/details/helpers.hpp"
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
#include <Helpers/Networking/NetLibrary.hpp>

#include "RichTextBox/RichTextBox.hpp"

std::shared_ptr<RichTextBox> textBox;
std::shared_ptr<RichTextBox::Cursor> textBoxCursor;
RichTextBox::PaintOpts paintOpts;

MainProgram::MainProgram():
    toolbar(*this)
{
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

    textBox = std::make_shared<RichTextBox>();
    textBoxCursor = std::make_shared<RichTextBox::Cursor>();
    textBox->set_width(1000.0f);
    textBox->set_font_collection(fonts.collection);
    textBox->insert(0, R"(
Lorem ipsum dolor sit amet consectetur adipiscing elit. Quisque faucibus ex sapien vitae pellentesque sem placerat. In id cursus mi pretium tellus duis convallis. Tempus leo eu aenean sed diam urna tempor. Pulvinar vivamus fringilla lacus nec metus bibendum egestas. Iaculis massa nisl malesuada lacinia integer nunc posuere. Ut hendrerit semper vel class aptent taciti sociosqu. Ad litora torquent per conubia nostra inceptos himenaeos.

Lorem ipsum dolor sit amet consectetur adipiscing elit. Quisque faucibus ex sapien vitae pellentesque sem placerat. In id cursus mi pretium tellus duis convallis. Tempus leo eu aenean sed diam urna tempor. Pulvinar vivamus fringilla lacus nec metus bibendum egestas. Iaculis massa nisl malesuada lacinia integer nunc posuere. Ut hendrerit semper vel class aptent taciti sociosqu. Ad litora torquent per conubia nostra inceptos himenaeos.

Lorem ipsum dolor sit amet consectetur adipiscing elit. Quisque faucibus ex sapien vitae pellentesque sem placerat. In id cursus mi pretium tellus duis convallis. Tempus leo eu aenean sed diam urna tempor. Pulvinar vivamus fringilla lacus nec metus bibendum egestas. Iaculis massa nisl malesuada lacinia integer nunc posuere. Ut hendrerit semper vel class aptent taciti sociosqu. Ad litora torquent per conubia nostra inceptos himenaeos.

Lorem ipsum dolor sit amet consectetur adipiscing elit. Quisque faucibus ex sapien vitae pellentesque sem placerat. In id cursus mi pretium tellus duis convallis. Tempus leo eu aenean sed diam urna tempor. Pulvinar vivamus fringilla lacus nec metus bibendum egestas. Iaculis massa nisl malesuada lacinia integer nunc posuere. Ut hendrerit semper vel class aptent taciti sociosqu. Ad litora torquent per conubia nostra inceptos himenaeos.

Lorem ipsum dolor sit amet consectetur adipiscing elit. Quisque faucibus ex sapien vitae pellentesque sem placerat. In id cursus mi pretium tellus duis convallis. Tempus leo eu aenean sed diam urna tempor. Pulvinar vivamus fringilla lacus nec metus bibendum egestas. Iaculis massa nisl malesuada lacinia integer nunc posuere. Ut hendrerit semper vel class aptent taciti sociosqu. Ad litora torquent per conubia nostra inceptos himenaeos.
    )");

    paintOpts.cursor = RichTextBox::Cursor{};
    paintOpts.cursorColor = {1.0f, 0.0f, 0.0f, 0.7f};
}

void MainProgram::update() {
    input.text.set_rich_text_box_input(textBox, textBoxCursor);

    //while(std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - lastFrameTime).count() < 1.0 / fpsLimit);
    //lastFrameTime = std::chrono::steady_clock::now();

    //if(input.key(InputManager::KEY_DRAW_TOOL_BRUSH).repeat)
    //    paintOpts.cursor.value().pos = textBox.move(paintOpts.cursor.value().pos, RichTextBox::Movement::kRight);
    //else if(input.key(InputManager::KEY_DRAW_TOOL_ERASER).repeat)
    //    paintOpts.cursor.value().pos = textBox.move(paintOpts.cursor.value().pos, RichTextBox::Movement::kLeft);
    //else if(input.key(InputManager::KEY_DRAW_TOOL_TEXTBOX).repeat)
    //    paintOpts.cursor.value().pos = textBox.move(paintOpts.cursor.value().pos, RichTextBox::Movement::kUp);
    //else if(input.key(InputManager::KEY_DRAW_TOOL_ELLIPSE).repeat)
    //    paintOpts.cursor.value().pos = textBox.move(paintOpts.cursor.value().pos, RichTextBox::Movement::kDown);

    //std::erase_if(input.droppedItems, [&](auto& droppedItem) {
    //    if(droppedItem.isFile && std::filesystem::is_regular_file(droppedItem.droppedData)) {
    //        std::filesystem::path droppedFilePath(droppedItem.droppedData);
    //        if(droppedFilePath.has_extension() && droppedFilePath.extension().string() == std::string("." + World::FILE_EXTENSION)) {
    //            new_tab({
    //                .conType = World::CONNECTIONTYPE_LOCAL,
    //                .fileSource = droppedFilePath.string()
    //            }, true);
    //            return true;
    //        }
    //    }
    //    return false;
    //});

    //if(setTabToClose) {
    //    worlds.erase(worlds.begin() + setTabToClose.value());
    //    setTabToClose.reset();
    //}

    //if(worldIndex >= worlds.size())
    //    worldIndex = worlds.size() - 1;
    //if(worlds.size() == 0)
    //    new_tab({
    //        .conType = World::CONNECTIONTYPE_LOCAL
    //    }, true);

    //std::shared_ptr<World> oldWorld = world;
    //world = worlds[worldIndex];

    //deltaTime.update_time_since();
    //deltaTime.update_time_point();

    //toolbar.initialize_io_before_update();

    //for(auto& w : worlds) {
    //    if(w == world)
    //        w->focus_update();
    //    else
    //        w->unfocus_update();
    //}

    //toolbar.update(); // GUI should be setup after the world data has been fully updated for this frame, so that the GUI reflects the current state of the world data

    //if(input.key(InputManager::KEY_NOGUI).pressed)
    //    drawGui = !drawGui;

    //if(input.key(InputManager::KEY_FULLSCREEN).pressed)
    //    input.toggleFullscreen = true;

    //NetLibrary::update();

    //if(tabSetToOpen) {
    //    tabSetToOpen = false;
    //    new_tab_open();
    //}

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
        Logger::get().log("WORLDFATAL", "Failed to open canvas: " + std::string(newTabToOpenInfo.fileSource) + " with error: " + e.what());
        return;
    }
    worlds.emplace_back(std::move(newWorld));
    worldIndex = worlds.size() - 1;
}

void MainProgram::set_tab_to_close(size_t tabToClose) {
    setTabToClose = tabToClose;
}

void MainProgram::save_config() {
    std::ofstream f(configPath / "config.json");
    if(f.is_open()) {
        using json = nlohmann::json;
        json j;

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
            try {
                toolbar.set_config_json(j["settings"]);
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
}

void MainProgram::draw(SkCanvas* canvas) {
    //canvas->clear(transparentBackground ? SkColor4f{0.0f, 0.0f, 0.0f, 0.0f} : world->canvasTheme.backColor);
    //world->draw(canvas);
    //toolbar.draw(canvas);

    canvas->clear(SkColor4f{0.0f, 0.0f, 0.0f, 1.0f});
    canvas->save();
    canvas->translate(200.0f, 200.0f);
    paintOpts.cursor = *textBoxCursor;
    textBox->paint(canvas, paintOpts);
    canvas->restore();
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
