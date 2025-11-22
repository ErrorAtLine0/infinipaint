#include "Toolbar.hpp"
#include "DrawingProgram/DrawingProgramToolBase.hpp"
#include "DrawingProgram/ScreenshotTool.hpp"
#include "FontData.hpp"
#include "Helpers/ConvertVec.hpp"
#include "Helpers/FileDownloader.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "MainProgram.hpp"
#include "GUIStuff/Elements/Element.hpp"
#include "GUIStuff/GUIManager.hpp"
#include "InputManager.hpp"
#include "VersionConstants.hpp"
#include "World.hpp"
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_render.h>
#include <algorithm>
#include <filesystem>
#include <optional>
#include <Helpers/Logger.hpp>
#include <fstream>
#include <Helpers/StringHelpers.hpp>
#include <Helpers/VersionNumber.hpp>

#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skparagraph/include/ParagraphStyle.h>
#include <modules/skparagraph/include/FontCollection.h>
#include <modules/skparagraph/include/TextStyle.h>
#include <include/core/SkFontStyle.h>
#include <modules/skunicode/include/SkUnicode_icu.h>

#include <modules/svg/include/SkSVGNode.h>
#include <include/core/SkStream.h>

#define UPDATE_DOWNLOAD_URL "https://infinipaint.com/download.html"
#define UPDATE_NOTIFICATION_URL "https://infinipaint.com/updateNotificationVersion.txt"

#ifdef __EMSCRIPTEN__
    #include <EmscriptenHelpers/emscripten_browser_file.h>
#endif

Toolbar::NativeFilePicker Toolbar::nativeFilePicker;

Toolbar::Toolbar(MainProgram& initMain):
    io(std::make_shared<GUIStuff::UpdateInputData>()),
    main(initMain)
{
    io->textTypeface = main.fonts->map["Roboto"];
    io->fonts = main.fonts;

    // NOTE: On windows, when the native file picker is open, any call to MakeFromFile fails
    // So, it's better to load the icons at the beginning of the program so that the icon loading doesn't fail later
    load_icons_at("data/icons");
    
    load_default_palette();
    load_default_theme();
}

void Toolbar::load_icons_at(const std::filesystem::path& pathToLoad) {
    for(auto& iconDirEntry : std::filesystem::recursive_directory_iterator(pathToLoad)) {
        if(iconDirEntry.is_regular_file()) {
            std::string iconRelativePath = iconDirEntry.path().relative_path().string();
            std::replace(iconRelativePath.begin(), iconRelativePath.end(), '\\', '/');
            auto stream = SkStream::MakeFromFile(iconRelativePath.c_str());
            if(!stream)
                throw std::runtime_error("[Toolbar::Toolbar] Could not open file " + iconRelativePath);
            auto svgDom = SkSVGDOM::Builder().make(*stream);
            if(!svgDom)
                throw std::runtime_error("[Toolbar::Toolbar] Could not parse SVG " + iconRelativePath);
            else {
                if(svgDom->containerSize().width() == 0 || svgDom->containerSize().height() == 0)
                    svgDom->setContainerSize({1000, 1000});
                io->svgData[iconRelativePath] = svgDom;
            }
        }
    }
}

void Toolbar::load_default_theme() {
    io->theme = GUIStuff::get_default_dark_mode();
    themeData.themeCurrentlyLoaded = "Default";
}

void Toolbar::save_theme() {
    std::filesystem::create_directory(main.configPath / "themes");
    std::ofstream f(main.configPath / "themes" / (themeData.themeCurrentlyLoaded + ".json"));
    if(f.is_open()) {
        using json = nlohmann::json;
        json j;
        j = *io->theme;
        f << j;
        f.close();
    }
}

bool Toolbar::load_theme() {
    std::filesystem::path themeDir = main.configPath / "themes";
    bool successfullyLoaded = false;
    if(std::filesystem::exists(themeDir) && std::filesystem::is_directory(themeDir)) {
        std::ifstream f(themeDir / (themeData.themeCurrentlyLoaded + ".json"));
        if(f.is_open()) {
            using json = nlohmann::json;
            try {
                json j;
                f >> j;
                auto theme(std::make_shared<GUIStuff::Theme>());
                j.get_to(*theme);
                io->theme = theme;
                successfullyLoaded = true;
            } catch(...) {}
            f.close();
        }
    }
    if(!successfullyLoaded)
        load_default_theme();
    return successfullyLoaded;
}

void Toolbar::load_default_palette() {
    paletteData.palettes.clear();
    paletteData.palettes.emplace_back();
    auto& palette = paletteData.palettes.back().colors;
    paletteData.palettes.back().name = "Default";
    palette = {{1.0,1.0,1.0},{0.0,0.0,0.0},{1.0,0.0,0.0},{1.0,0.529411792755127,0.0},{1.0,0.8274509906768799,0.0},{0.8705882430076599,1.0,0.03921568766236305},{0.6313725709915161,1.0,0.03921568766236305},{0.03921568766236305,1.0,0.6000000238418579},{0.03921568766236305,0.9372549057006836,1.0},{0.0784313753247261,0.4901960790157318,0.9607843160629272},{0.3450980484485626,0.03921568766236305,1.0},{0.7450980544090271,0.03921568766236305,1.0}};

    paletteData.selectedPalette = 0;
}

void Toolbar::save_palettes() {
    std::ofstream f(main.configPath / "palettes.json");
    if(f.is_open()) {
        using json = nlohmann::json;
        json j;
        auto palettesToSave = paletteData.palettes;
        palettesToSave.erase(palettesToSave.begin());
        j = palettesToSave;
        f << j;
        f.close();
    }
}

void Toolbar::load_palettes() {
    std::ifstream f(main.configPath / "palettes.json");
    load_default_palette();
    if(f.is_open()) {
        using json = nlohmann::json;
        try {
            json j;
            f >> j;
            std::vector<PaletteData::Palette> palettes;
            j.get_to(palettes);
            paletteData.palettes.insert(paletteData.palettes.end(), palettes.begin(), palettes.end());
        } catch(...) {}
        f.close();
    }
}

// You can't trust that filter wont be -1 on the platform youre on, so dont use the extension from the callback
void Toolbar::sdl_open_file_dialog_callback(void* userData, const char * const * fileList, int filter) {
    if(!fileList) {
        nativeFilePicker.isOpen = false;
        return;
    }
    if(!(*fileList)) {
        nativeFilePicker.isOpen = false;
        return;
    }
    ExtensionFilter e;
    if(filter >= 0)
        e = nativeFilePicker.extensionFiltersComplete[filter];
    nativeFilePicker.postSelectionFunc(fileList[0], e);
    nativeFilePicker.isOpen = false;
}

void Toolbar::open_file_selector(const std::string& filePickerName, const std::vector<ExtensionFilter>& extensionFilters, OpenFileSelectorCallback postSelectionFunc, const std::string& fileName, bool isSaving) {
    if(useNativeFilePicker) {
        if(!nativeFilePicker.isOpen) {
            nativeFilePicker.postSelectionFunc = postSelectionFunc;
            nativeFilePicker.extensionFiltersComplete = extensionFilters;
            nativeFilePicker.sdlFileFilters.clear();
            for(auto& e : nativeFilePicker.extensionFiltersComplete)
                nativeFilePicker.sdlFileFilters.emplace_back(e.name.c_str(), e.extensions.c_str());
            if(isSaving)
                SDL_ShowSaveFileDialog(sdl_open_file_dialog_callback, nullptr, main.window.sdlWindow, nativeFilePicker.sdlFileFilters.data(), nativeFilePicker.sdlFileFilters.size(), nullptr);
            else
                SDL_ShowOpenFileDialog(sdl_open_file_dialog_callback, nullptr, main.window.sdlWindow, nativeFilePicker.sdlFileFilters.data(), nativeFilePicker.sdlFileFilters.size(), nullptr, false);
        }
    }
    else {
        filePicker.isOpen = true;
        filePicker.refreshEntries = true;
        filePicker.extensionFiltersComplete = extensionFilters;
        filePicker.extensionFilters.clear();
        for(auto& [name, exList] : extensionFilters)
            filePicker.extensionFilters.emplace_back(exList);
        filePicker.extensionSelected = extensionFilters.size() - 1;
        filePicker.filePickerWindowName = filePickerName;
        filePicker.postSelectionFunc = postSelectionFunc;
        filePicker.fileName = "";
    }
}

std::filesystem::path& Toolbar::file_selector_path() {
    return filePicker.currentSearchPath;
}

void Toolbar::color_selector_left(Vector4f* color) {
    colorLeft = color;
    justAssignedColorLeft = true;
}

void Toolbar::color_selector_right(Vector4f* color) {
    colorRight = color;
    justAssignedColorRight = true;
}

void Toolbar::load_licenses() {
    std::filesystem::path third_party_license_path("data/third_party_licenses");
    for(const auto& entry : std::filesystem::directory_iterator(third_party_license_path)) {
        if(entry.is_regular_file())
            thirdPartyLicenses.emplace_back(entry.path().filename().string(), read_file_to_string(entry.path()));
    }
    std::sort(thirdPartyLicenses.begin(), thirdPartyLicenses.end(), [](const auto& a1, const auto& a2) {
        return std::lexicographical_compare(a1.first.begin(), a1.first.end(), a2.first.begin(), a2.first.end());
    });
    ownLicenseText = "InfiniPaint v" + VersionConstants::CURRENT_VERSION_STRING;
    ownLicenseText +=
R"(

Copyright © 2025 Yousef Khadadeh

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
)";
}

nlohmann::json Toolbar::get_config_json() {
    using json = nlohmann::json;
    json toRet;

    json jKeybinds;
    for(unsigned i = 0; i < InputManager::KEY_ASSIGNABLE_COUNT; i++) {
        auto f = std::find_if(main.input.keyAssignments.begin(), main.input.keyAssignments.end(), [&](auto& p) {
            return p.second == i;
        });
        if(f != main.input.keyAssignments.end())
            jKeybinds[json(static_cast<InputManager::KeyCodeEnum>(i))] = main.input.key_assignment_to_str(f->first);
    }
    toRet["keybinds"] = jKeybinds;
    toRet["guiScale"] = guiScale;
    toRet["jumpTransitionTime"] = jumpTransitionTime;
    toRet["dragZoomSpeed"] = dragZoomSpeed;
    toRet["scrollZoomSpeed"] = scrollZoomSpeed;
    toRet["vsync"] = main.window.vsyncValue;
    toRet["showPerformance"] = showPerformance;
    toRet["displayName"] = main.displayName;
    toRet["useNativeFilePicker"] = useNativeFilePicker;
    toRet["themeInUse"] = themeData.themeCurrentlyLoaded;
    toRet["velocityAffectsBrushWidth"] = velocityAffectsBrushWidth;
    toRet["jumpTransitionEasing"] = jumpTransitionEasing;
    toRet["defaultCanvasBackgroundColor"] = main.defaultCanvasBackgroundColor;
    toRet["flipZoomToolDirection"] = flipZoomToolDirection;
#ifndef __EMSCRIPTEN__
    toRet["checkForUpdates"] = updateCheckerData.checkForUpdates;
#endif

    json tablet;
    tablet["pressureAffectsBrushWidth"] = tabletOptions.pressureAffectsBrushWidth;
    tablet["smoothingSamplingTime"] = tabletOptions.smoothingSamplingTime;
    tablet["middleClickButton"] = tabletOptions.middleClickButton;
    tablet["rightClickButton"] = tabletOptions.rightClickButton;
    tablet["ignoreMouseMovementWhenPenInProximity"] = tabletOptions.ignoreMouseMovementWhenPenInProximity;

    toRet["tablet"] = tablet;

    return toRet;
}

void Toolbar::set_config_json(const nlohmann::json& j, VersionNumber version) {
    using json = nlohmann::json;
    main.input.keyAssignments.clear();
    try {
        const json& jKeybinds = j.at("keybinds");
        for(unsigned i = 0; i < InputManager::KEY_ASSIGNABLE_COUNT; i++) {
            try {
                Vector2ui32 a = main.input.key_assignment_from_str(jKeybinds.at(json(static_cast<InputManager::KeyCodeEnum>(i))));
                if(a != Vector2ui32{0, 0})
                    main.input.keyAssignments.emplace(a, i);
                else
                    throw;
            }
            catch(...) {
                auto f = std::find_if(main.input.defaultKeyAssignments.begin(), main.input.defaultKeyAssignments.end(), [&](auto& p) {
                    return p.second == i;
                });
                main.input.keyAssignments.emplace(f->first, i);
            }
        }
    }
    catch(...) {
        main.input.keyAssignments = main.input.defaultKeyAssignments;
    }
    try{j.at("displayName").get_to(main.displayName);} catch(...) {}
    try{j.at("jumpTransitionTime").get_to(jumpTransitionTime);} catch(...) {}
    try{j.at("dragZoomSpeed").get_to(dragZoomSpeed);} catch(...) {}
    try{j.at("scrollZoomSpeed").get_to(scrollZoomSpeed);} catch(...) {}
    try {
        j.at("vsync").get_to(main.window.vsyncValue);
        main.set_vsync_value(main.window.vsyncValue);
    }
    catch(...) {
        main.set_vsync_value(1);
    }
    try{j.at("guiScale").get_to(guiScale);} catch(...) {}
    try{j.at("showPerformance").get_to(showPerformance);} catch(...) {}
    try{j.at("useNativeFilePicker").get_to(useNativeFilePicker);} catch(...) {}
    try{j.at("themeInUse").get_to(themeData.themeCurrentlyLoaded);} catch(...) {}
    try{j.at("velocityAffectsBrushWidth").get_to(velocityAffectsBrushWidth);} catch(...) {}
    try{j.at("jumpTransitionEasing").get_to(jumpTransitionEasing);} catch(...) {}
    if(version >= VersionNumber(0, 3, 0))
        try{j.at("defaultCanvasBackgroundColor").get_to(main.defaultCanvasBackgroundColor);} catch(...) {}
    try{j.at("flipZoomToolDirection").get_to(flipZoomToolDirection);} catch(...) {}
#ifndef __EMSCRIPTEN__
    try{j.at("checkForUpdates").get_to(updateCheckerData.checkForUpdates);} catch(...) {}
#endif

    try{j.at("tablet").at("pressureAffectsBrushWidth").get_to(tabletOptions.pressureAffectsBrushWidth);} catch(...) {}
    try{j.at("tablet").at("smoothingSamplingTime").get_to(tabletOptions.smoothingSamplingTime);} catch(...) {}
    try{j.at("tablet").at("middleClickButton").get_to(tabletOptions.middleClickButton);} catch(...) {}
    try{j.at("tablet").at("rightClickButton").get_to(tabletOptions.rightClickButton);} catch(...) {}
    try{j.at("tablet").at("ignoreMouseMovementWhenPenInProximity").get_to(tabletOptions.ignoreMouseMovementWhenPenInProximity);} catch(...) {}

    main.update_display_names();
}

void Toolbar::update() {
#ifndef __EMSCRIPTEN__
    update_notification_check();
#endif

    if(main.input.key(InputManager::KEY_SAVE).pressed && !optionsMenuOpen && !filePicker.isOpen)
        save_func();
    if(main.input.key(InputManager::KEY_SAVE_AS).pressed && !optionsMenuOpen && !filePicker.isOpen)
        save_as_func();
    if(main.input.key(InputManager::KEY_SHOW_METRICS).pressed)
        showPerformance = !showPerformance;
    if(main.input.key(InputManager::KEY_OPEN_CHAT).pressed && chatBoxState == CHATBOXSTATE_CLOSE)
        chatBoxState = CHATBOXSTATE_JUSTOPEN;
    if(main.input.key(InputManager::KEY_SHOW_PLAYER_LIST).pressed)
        playerMenuOpen = !playerMenuOpen;

    start_gui();

    if(main.drawGui) {
        CLAY({
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIT(gui.windowSize.x()), .height = CLAY_SIZING_FIT(gui.windowSize.y())},
                .padding = CLAY_PADDING_ALL(io->theme->padding1),
                .childGap = io->theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }
        }) {
            top_toolbar();
            drawing_program_gui();
            if(viewWebVersionWelcome)
                web_version_welcome();
#ifndef __EMSCRIPTEN__
            else if(updateCheckerData.showGui)
                update_notification_gui();
#endif
            else if(filePicker.isOpen)
                file_picker_gui();
            else if(optionsMenuOpen)
                options_menu();
            else if(playerMenuOpen)
                player_list();
            if(showPerformance)
                performance_metrics();
        }
        chat_box();
    }

    if(io->hoverObstructed && (io->mouse.leftClick || io->mouse.rightClick))
        paintPopupLocation = std::nullopt;

    paint_popup();

    if(io->hoverObstructed && io->mouse.rightClick) // This runs specifically if the paint popup has the cursor
        paintPopupLocation = std::nullopt;

    end_gui();

    justAssignedColorLeft = false;
    justAssignedColorRight = false;

    if(!optionsMenuOpen || generalSettingsOptions != GSETTINGS_KEYBINDS)
        keybindWaiting = std::nullopt;

    calculate_final_gui_scale();
}

void Toolbar::save_func() {
    if(main.world->filePath == std::filesystem::path())
        save_as_func();
    else
        main.world->save_to_file(main.world->filePath.string());
}

void Toolbar::save_as_func() {
    #ifdef __EMSCRIPTEN__
        optionsMenuOpen = true;
        optionsMenuType = SET_DOWNLOAD_NAME;
    #else
        open_file_selector("Save", {{"Any File", "*"}, {"InfiniPaint Canvas", World::FILE_EXTENSION}}, [w = make_weak_ptr(main.world)](const std::filesystem::path& p, const auto& e) {
            auto world = w.lock();
            if(world)
                world->save_to_file(p);
        }, "", true);
    #endif
}

void Toolbar::paint_popup() {
    if(paintPopupLocation) {
        double newRotationAngle = 0.0;
        gui.paint_circle_popup_menu("paint circle popup", paintPopupLocation.value(), {
            .currentRotationAngle = main.world->drawData.cam.c.rotation,
            .newRotationAngle = &newRotationAngle,
            .selectedColor = main.world->drawProg.get_foreground_color_ptr(),
            .palette = paletteData.palettes[paletteData.selectedPalette].colors
        });
        if(newRotationAngle != main.world->drawData.cam.c.rotation) {
            main.world->drawData.cam.c.rotate_about(main.world->drawData.cam.c.from_space(main.window.size.cast<float>() * 0.5f), newRotationAngle - main.world->drawData.cam.c.rotation);
        }
    }
}

void Toolbar::top_toolbar() {
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(static_cast<uint16_t>(io->theme->padding1 / 2)),
            .childGap = static_cast<uint16_t>(io->theme->childGap1 / 2),
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
        .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1)
    }) {
        gui.obstructing_window();
        gui.push_id("menu top toolbar");
        global_log();
        bool menuPopUpJustOpen = false;
        bool bookmarkMenuPopUpJustOpen = false;
        bool gridMenuPopUpJustOpen = false;
        if(gui.svg_icon_button_transparent("Main Menu Button", "data/icons/menu.svg", menuPopUpOpen)) {
            menuPopUpOpen = true;
            menuPopUpJustOpen = true;
        }
        std::vector<std::pair<std::string, std::string>> tabNames;
        for(size_t i = 0; i < main.worlds.size(); i++)
            tabNames.emplace_back(main.worlds[i]->network_being_used() ? "data/icons/network.svg" : "", main.worlds[i]->name);
        std::optional<size_t> closedTab;
        gui.tab_list("file tab list", tabNames, main.worldIndex, closedTab);
        if(main.world->network_being_used() && gui.svg_icon_button_transparent("Player List Toggle Button", "data/icons/list.svg", playerMenuOpen))
            playerMenuOpen = !playerMenuOpen;
        if(gui.svg_icon_button_transparent("Menu Undo Button", "data/icons/undo.svg"))
            main.world->undo_with_checks();
        if(gui.svg_icon_button_transparent("Menu Redo Button", "data/icons/redo.svg"))
            main.world->redo_with_checks();
        if(gui.svg_icon_button_transparent("Grids Button", "data/icons/grid.svg", gridMenu.popupOpen)) {
            if(gridMenu.popupOpen)
                stop_displaying_grid_menu();
            else {
                gridMenu.popupOpen = true;
                gridMenuPopUpJustOpen = true;
            }
        }
        if(gui.svg_icon_button_transparent("Bookmark Menu Button", "data/icons/bookmark.svg", bookMenu.popupOpen)) {
            if(bookMenu.popupOpen)
                bookMenu.popupOpen = false;
            else {
                bookMenu.popupOpen = true;
                bookmarkMenuPopUpJustOpen = true;
            }
        }


        if(closedTab)
            main.set_tab_to_close(closedTab.value());
        if(gridMenu.popupOpen)
            grid_menu(gridMenuPopUpJustOpen);
        if(bookMenu.popupOpen)
            bookmark_menu(bookmarkMenuPopUpJustOpen);
        if(menuPopUpOpen) {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(100), .height = CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                    .childGap = 1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
                .floating = {.offset = {.x = 0, .y = static_cast<float>(io->theme->padding1)}, .attachPoints = {.element = CLAY_ATTACH_POINT_LEFT_TOP, .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
            }) {
                gui.obstructing_window();
                if(gui.text_button_left_transparent("new file local", "New File")) {
                    main.new_tab({
                        .conType = World::CONNECTIONTYPE_LOCAL
                    }, true);
                }
                if(gui.text_button_left_transparent("save file", "Save"))
                    save_func();
                if(gui.text_button_left_transparent("save as file", "Save As"))
                    save_as_func();
                if(gui.text_button_left_transparent("open file", "Open"))
                    open_world_file(World::CONNECTIONTYPE_LOCAL, "", "");
                if(gui.text_button_left_transparent("screenshot", "Take Screenshot"))
                    main.world->drawProg.switch_to_tool(DrawingProgramToolType::SCREENSHOT);
                if(gui.text_button_left_transparent("add image or file to canvas", "Add Image/File to Canvas")) {
                    #ifdef __EMSCRIPTEN__
                        emscripten_browser_file::upload("*", [](std::string const& fileName, std::string const& mimeType, std::string_view buffer, void* callbackData) {
                            if(!buffer.empty()) {
                                World* world = (World*)callbackData;
                                world->drawProg.add_file_to_canvas_by_data(fileName, buffer, world->main.window.size.cast<float>() / 2.0f);
                            }
                        }, main.world.get());
                    #else
                        open_file_selector("Open File", {{"Any File", "*"}}, [w = make_weak_ptr(main.world)](const std::filesystem::path& p, const auto& e) {
                            auto wLock = w.lock();
                            if(wLock)
                                wLock->drawProg.add_file_to_canvas_by_path(p.string(), wLock->main.window.size.cast<float>() / 2.0f, false);
                        });
                    #endif
                }
                if(main.world->network_being_used()) {
                    if(gui.text_button_left_transparent("lobby info", "Lobby Info")) {
                        optionsMenuOpen = true;
                        optionsMenuType = LOBBY_INFO_MENU;
                    }
                }
                else if(gui.text_button_left_transparent("start hosting", "Host")) {
                    serverLocalID = NetLibrary::get_random_server_local_id();
                    serverToConnectTo = NetLibrary::get_global_id() + serverLocalID;
                    optionsMenuOpen = true;
                    optionsMenuType = HOST_MENU;
                }
                if(gui.text_button_left_transparent("start connecting", "Connect")) {
                    serverToConnectTo.clear();
                    optionsMenuOpen = true;
                    optionsMenuType = CONNECT_MENU;
                }
                if(gui.text_button_left_transparent("canvas specific settings", "Canvas Settings")) {
                    optionsMenuOpen = true;
                    optionsMenuType = CANVAS_SETTINGS_MENU;
                }
                if(gui.text_button_left_transparent("open options", "Settings")) {
                    optionsMenuOpen = true;
                    optionsMenuType = GENERAL_SETTINGS_MENU;
                }
                if(gui.text_button_left_transparent("about menu button", "About")) {
                    optionsMenuOpen = true;
                    optionsMenuType = ABOUT_MENU;
                }
                #ifndef __EMSCRIPTEN__
                    if(gui.text_button_left_transparent("quit button", "Quit"))
                        main.setToQuit = true;
                #endif
                if(io->mouse.leftClick && !menuPopUpJustOpen)
                    menuPopUpOpen = false;
            }
        }
        gui.pop_id();
    }
}

void Toolbar::web_version_welcome() {
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(700), .height = CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
        .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        gui.obstructing_window();
        gui.push_id("web version welcome gui");
        gui.text_label_centered("Welcome to the web version of InfiniPaint!");
        gui.text_label(
R"(This version contains more known issues than the native version of the app. This includes:
- Rare crashes
- If this browser tab is unfocused, or the window is minimized, any InfiniPaint tabs connected online (whether host or client) will be disconnected
- 4GB memory limit. Might be a problem if you're uploading many files/images
- Not multithreaded
- Can't access local fonts

If you like this app, consider downloading the native version for your system)");
        if(gui.text_button_wide("got it", "Got It"))
            viewWebVersionWelcome = false;
        gui.pop_id();
    }
}

#ifndef __EMSCRIPTEN__
void Toolbar::update_notification_check() {
    if(!updateCheckerData.updateCheckDone) {
        if(updateCheckerData.checkForUpdates) {
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
                            Logger::get().log("INFO", "Latest online version is v" + updateCheckerData.newVersionStr);
                            if(newV > currentV)
                                updateCheckerData.showGui = true;
                            else if(newV == currentV)
                                Logger::get().log("INFO", "Current version is up to date");
                            else
                                Logger::get().log("INFO", "Local version has larger version number than the latest online one");
                        }
                        else
                            Logger::get().log("INFO", "Update notification file couldn't be converted to version numbers");
                        updateCheckerData.versionFile = nullptr;
                        break;
                    }
                    case FileDownloader::DownloadData::Status::FAILURE:
                        Logger::get().log("INFO", "Failed to check for updates");
                        updateCheckerData.updateCheckDone = true;
                        updateCheckerData.versionFile = nullptr;
                        break;
                }
            }
        }
        else
            updateCheckerData.updateCheckDone = true;
    }
}

void Toolbar::update_notification_gui() {
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(700), .height = CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
        .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        gui.obstructing_window();
        gui.push_id("update notification gui");
        gui.text_label_centered("Update v" + updateCheckerData.newVersionStr + " available!");
        if(gui.text_button_wide("download", "Open download page in web browser")) {
            SDL_OpenURL(UPDATE_DOWNLOAD_URL);
            updateCheckerData.showGui = false;
        }
        if(gui.text_button_wide("ignore forever", "Ignore and don't notify again (can be changed in settings)")) {
            updateCheckerData.checkForUpdates = false;
            updateCheckerData.showGui = false;
        }
        if(gui.text_button_wide("ignore for now", "Ignore for now"))
           updateCheckerData.showGui = false;
        gui.pop_id();
    }
}
#endif

void Toolbar::grid_menu(bool justOpened) {
    gui.push_id("grid menu");
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0, 600) },
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
        .floating = {.offset = {.x = 0, .y = static_cast<float>(io->theme->padding1)}, .attachPoints = {.element = CLAY_ATTACH_POINT_RIGHT_TOP, .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        gui.obstructing_window();
        gui.text_label_centered("Grids");
        float entryHeight = 25.0f;
        if(main.world->gridMan.sorted_grid_ids().empty())
            gui.text_label_centered("No grids yet...");
        ServerClientID toDelete{0, 0};
        gui.scroll_bar_many_entries_area("grid menu entries", entryHeight, main.world->gridMan.sorted_grid_ids().size(), [&](size_t i, bool isListHovered) {
            ServerClientID gridID = main.world->gridMan.sorted_grid_ids()[i];
            WorldGrid& grid = main.world->gridMan.grids[gridID];
            bool selectedEntry = gridID == gridMenu.gridSelected;
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(entryHeight)},
                    .childGap = 2,
                    .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT 
                },
                .backgroundColor = selectedEntry ? convert_vec4<Clay_Color>(io->theme->backColor1) : convert_vec4<Clay_Color>(io->theme->backColor2)
            }) {
                gui.text_label(grid.get_display_name());
                bool miniButtonClicked = false;
                CLAY({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                        .childGap = 1,
                        .childAlignment = {.x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                    }
                }) {
                    if(gui.svg_icon_button_transparent("visibility eye", grid.visible ? "data/icons/eyeopen.svg" : "data/icons/eyeclose.svg", false, entryHeight, false)) {
                        miniButtonClicked = true;
                        WorldGrid oldGrid = grid;
                        grid.visible = !grid.visible;
                        main.world->gridMan.send_grid_info(oldGrid, gridID);
                    }
                    if(gui.svg_icon_button_transparent("edit pencil", "data/icons/pencil.svg", false, entryHeight, false)) {
                        miniButtonClicked = true;
                        main.world->drawProg.modify_grid(gridID);
                        stop_displaying_grid_menu();
                    }
                    if(gui.svg_icon_button_transparent("delete trash", "data/icons/trash.svg", false, entryHeight, false)) {
                        miniButtonClicked = true;
                        toDelete = gridID;
                    }
                }
                if(Clay_Hovered() && io->mouse.leftClick && isListHovered && !miniButtonClicked) {
                    gridMenu.gridSelected = gridID;
                    if(io->mouse.leftClick >= 2) {
                        main.world->drawProg.modify_grid(gridID);
                        stop_displaying_grid_menu();
                    }
                }
            }
        });
        if(toDelete != ServerClientID{0, 0})
            main.world->gridMan.remove_grid(toDelete);
        gui.left_to_right_line_layout([&]() {
            bool addByEnter = false;
            gui.input_text("grid text input", &gridMenu.newName, true, [&](GUIStuff::SelectionHelper& s) {
                addByEnter = s.selected && io->key.enter;
            });
            if(gui.svg_icon_button("grid add button", "data/icons/plus.svg", false, GUIStuff::GUIManager::SMALL_BUTTON_SIZE) || (addByEnter && !gridMenu.newName.empty())) {
                main.world->drawProg.modify_grid(main.world->gridMan.add_default_grid(gridMenu.newName));
                stop_displaying_grid_menu();
            }
        });

        bool dropdownHover = false;
        if(io->mouse.leftClick && !Clay_Hovered() && !justOpened && !dropdownHover)
            stop_displaying_grid_menu();
    }
    gui.pop_id();
}

void Toolbar::stop_displaying_grid_menu() {
    gridMenu.newName.clear();
    gridMenu.popupOpen = false;
    gridMenu.gridSelected = ServerClientID{0, 0};
}

void Toolbar::bookmark_menu(bool justOpened) {
    gui.push_id("bookmark menu");
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0, 600) },
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
        .floating = {.offset = {.x = 0, .y = static_cast<float>(io->theme->padding1)}, .attachPoints = {.element = CLAY_ATTACH_POINT_RIGHT_TOP, .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        gui.obstructing_window();
        float entryHeight = 25.0f;
        gui.text_label_centered("Bookmarks");
        if(main.world->bMan.sorted_names().empty())
            gui.text_label_centered("No bookmarks yet...");
        std::string toDelete;
        gui.scroll_bar_many_entries_area("bookmark menu entries", entryHeight, main.world->bMan.sorted_names().size(), [&](size_t i, bool isListHovered) {
            const std::string& bookmark = main.world->bMan.sorted_names()[i];
            bool selectedEntry = bookmark == bookMenu.newName;
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(entryHeight)},
                    .childGap = 2,
                    .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT 
                },
                .backgroundColor = selectedEntry ? convert_vec4<Clay_Color>(io->theme->backColor1) : convert_vec4<Clay_Color>(io->theme->backColor2)
            }) {
                gui.text_label(bookmark);
                bool miniButtonClicked = false;
                CLAY({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                        .childGap = 1,
                        .childAlignment = {.x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                    }
                }) {
                    if(gui.svg_icon_button_transparent("bookmark jump button", "data/icons/jump.svg", false, entryHeight, false)) {
                        miniButtonClicked = true;
                        main.world->bMan.jump_to_bookmark(bookmark);
                    }
                    if(gui.svg_icon_button_transparent("delete trash", "data/icons/trash.svg", false, entryHeight, false)) {
                        miniButtonClicked = true;
                        toDelete = bookmark;
                    }
                }
                if(Clay_Hovered() && io->mouse.leftClick && isListHovered && !miniButtonClicked) {
                    bookMenu.newName = bookmark;
                    if(io->mouse.leftClick >= 2)
                        main.world->bMan.jump_to_bookmark(bookmark);
                }
            }
        });
        if(!toDelete.empty())
            main.world->bMan.remove_bookmark(toDelete);
        bool bookmarkExists = std::find(main.world->bMan.sorted_names().begin(), main.world->bMan.sorted_names().end(), bookMenu.newName) != main.world->bMan.sorted_names().end();
        gui.left_to_right_line_layout([&]() {
            bool addByEnter = false;
            gui.input_text("bookmark text input", &bookMenu.newName, true, [&](GUIStuff::SelectionHelper& s) {
                addByEnter = s.selected && io->key.enter;
            });
            if(!bookmarkExists) {
                if((gui.svg_icon_button("bookmark add button", "data/icons/plus.svg", false, GUIStuff::GUIManager::SMALL_BUTTON_SIZE) || addByEnter) && !bookMenu.newName.empty())
                    main.world->bMan.add_bookmark(bookMenu.newName);
            }
        });
        if(io->mouse.leftClick && !Clay_Hovered() && !justOpened) {
            bookMenu.newName.clear();
            bookMenu.popupOpen = false;
        }
    }
    gui.pop_id();
}

std::unique_ptr<skia::textlayout::Paragraph> Toolbar::build_paragraph_from_chat_message(const ChatMessage& message, float alpha) {
    skia::textlayout::ParagraphStyle pStyle;
    pStyle.setTextAlign(skia::textlayout::TextAlign::kLeft);
    skia::textlayout::TextStyle tStyle;
    tStyle.setFontSize(io->fontSize);
    tStyle.setFontFamilies(main.fonts->get_default_font_families());
    tStyle.setFontStyle(SkFontStyle::Bold());
    tStyle.setForegroundColor(SkPaint{color_mul_alpha(message.type == ChatMessage::JOIN ? io->theme->warningColor : io->theme->frontColor1, alpha)});
    pStyle.setTextStyle(tStyle);

    skia::textlayout::ParagraphBuilderImpl a(pStyle, io->fonts->collection, SkUnicodes::ICU::Make());
    if(message.type == ChatMessage::JOIN) {
        std::string messageName = message.name + " ";
        a.addText(messageName.c_str(), messageName.length());
    }
    else {
        std::string messageName = "[" + message.name + "] ";
        a.addText(messageName.c_str(), messageName.length());
    }

    tStyle.setFontStyle(SkFontStyle::Normal());
    a.pushStyle(tStyle);
    a.addText(message.message.c_str(), message.message.length());

    return a.Build();
}

void Toolbar::chat_box() {
    constexpr float CHATBOX_WIDTH = 700;
    if(main.world->network_being_used()) {
        CLAY({
            .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            },
            .floating = {.offset = {static_cast<float>(io->theme->padding1), -static_cast<float>(io->theme->padding1)}, .attachPoints = {.element = CLAY_ATTACH_POINT_LEFT_BOTTOM, .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
        }) {
            gui.obstructing_window();
            gui.push_id("chat box open button");
            if(gui.svg_icon_button("Chat Open Button", "data/icons/chat.svg", chatBoxState != CHATBOXSTATE_CLOSE)) {
                if(chatBoxState == CHATBOXSTATE_CLOSE)
                    chatBoxState = CHATBOXSTATE_JUSTOPEN;
                else
                    chatBoxState = CHATBOXSTATE_CLOSE;
            }
            gui.pop_id();
        }
    }
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(CHATBOX_WIDTH), .height = CLAY_SIZING_FIT(0) },
            .childGap = 0,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .floating = {.offset = {60 + static_cast<float>(io->theme->padding1), -static_cast<float>(io->theme->padding1)}, .attachPoints = {.element = CLAY_ATTACH_POINT_LEFT_BOTTOM, .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        gui.obstructing_window();
        gui.push_id("chat box");
        if(chatBoxState == CHATBOXSTATE_JUSTOPEN || chatBoxState == CHATBOXSTATE_OPEN) {
            gui.push_id("messages");
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(0),
                    .childGap = 0,
                    .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1)
            }) {
                int id = 0;
                for(auto& chatMessage : main.world->chatMessages | std::views::reverse) {
                    gui.push_id(id);
                    gui.text_paragraph("text", build_paragraph_from_chat_message(chatMessage, 1.0f), CHATBOX_WIDTH);
                    gui.pop_id();
                    id++;
                }
            }
            gui.pop_id();

            gui.left_to_right_line_layout([&]() {
                gui.input_text("message input", &chatMessageInput);
                if(gui.text_button("send button", "Send")) {
                    if(!chatMessageInput.empty())
                        main.world->send_chat_message(chatMessageInput);
                }
            });
            gui.push_id("message input");
            GUIStuff::GUIManager::ElementContainer& a = gui.elements[gui.idStack];
            GUIStuff::TextBox<std::string>& t = *(GUIStuff::TextBox<std::string>*)a.elem.get();
            gui.pop_id();
            if(chatBoxState == CHATBOXSTATE_JUSTOPEN) {
                t.selection.selected = true;
                chatBoxState = CHATBOXSTATE_OPEN;
            }
            if(io->key.escape)
                t.selection.selected = false;
            if(io->key.enter) {
                main.world->send_chat_message(chatMessageInput);
                t.selection.selected = false;
            }
            if(!t.selection.selected) {
                chatMessageInput.clear();
                chatBoxState = CHATBOXSTATE_CLOSE;
            }
        }
        else {
            constexpr float DISPLAY_TIME = 8.0f;
            constexpr float FADE_START_TIME = 7.0f;
            gui.push_id("messages popup");
            int id = 0;
            for(auto& chatMessage : main.world->chatMessages | std::views::reverse) {
                chatMessage.time.update_time_since();
                if(chatMessage.time < DISPLAY_TIME) {
                    float a = 1.0f - lerp_time<float>(chatMessage.time, DISPLAY_TIME, FADE_START_TIME);
                    CLAY({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                            .padding = CLAY_PADDING_ALL(0),
                            .childGap = 0,
                            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        },
                        .backgroundColor = convert_vec4<Clay_Color>(color_mul_alpha(io->theme->backColor1, a)),
                    }) {
                        gui.push_id(id);
                        gui.obstructing_window();
                        gui.text_paragraph("text", build_paragraph_from_chat_message(chatMessage, a), CHATBOX_WIDTH);
                        gui.pop_id();
                    }
                }
                id++;
            }
            gui.pop_id();
        }
        gui.pop_id();
    }
}

void Toolbar::global_log() {
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_FIT(0) },
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .floating = {.offset = {0, 10}, .attachPoints = {.element = CLAY_ATTACH_POINT_RIGHT_TOP, .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        constexpr float DISPLAY_TIME = 8.0f;
        constexpr float FADE_START_TIME = 7.0f;
        for(auto& logM : main.logMessages) {
            logM.time.update_time_since();
            if(logM.time < DISPLAY_TIME) {
                float a = 1.0f - lerp_time<float>(logM.time, DISPLAY_TIME, FADE_START_TIME);
                CLAY({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0) },
                        .padding = CLAY_PADDING_ALL(io->theme->padding1),
                        .childGap = 0,
                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM
                    },
                    .backgroundColor = convert_vec4<Clay_Color>(color_mul_alpha(io->theme->backColor1, a)),
                    .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1)
                }) {
                    gui.obstructing_window();
                    SkColor4f c{0, 0, 0, 0};
                    switch(logM.color) {
                        case LogMessage::COLOR_NORMAL:
                            c = io->theme->frontColor1;
                            break;
                        case LogMessage::COLOR_ERROR:
                            c = io->theme->errorColor;
                            break;
                    }
                    gui.text_label_color(logM.text, color_mul_alpha(c, a));
                }
            }
            else
                break;
        }
    }
}

void Toolbar::drawing_program_gui() {
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
    }) {
        main.world->drawProg.toolbar_gui();
        isUpdatingColorLeft = isUpdatingColorRight = false;
        if(colorLeft) {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0)},
                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                    .childGap = io->theme->childGap1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1)
            }) {
                gui.obstructing_window();
                isUpdatingColorLeft |= gui.color_picker_items("colorpickerleft", colorLeft, true);
                bool hoveringOnDropdown = false;
                isUpdatingColorLeft |= color_palette("colorpickerleftpalette", colorLeft, hoveringOnDropdown);
                if(!Clay_Hovered() && !justAssignedColorLeft && !hoveringOnDropdown && io->mouse.leftClick)
                    colorLeft = nullptr;
            }
        }
        CLAY({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
            }
        }) {}
        if(colorRight) {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0)},
                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                    .childGap = io->theme->childGap1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1)
            }) {
                gui.obstructing_window();
                isUpdatingColorRight |= gui.color_picker_items("colorpickerright", colorRight, true);
                bool hoveringOnDropdown = false;
                isUpdatingColorRight |= color_palette("colorpickerrightpalette", colorRight, hoveringOnDropdown);
                if(!Clay_Hovered() && !justAssignedColorRight && !hoveringOnDropdown && io->mouse.leftClick)
                    colorRight = nullptr;
            }
        }
        main.world->drawProg.tool_options_gui();
    }
}

bool Toolbar::color_palette(const std::string& id, Vector4f* color, bool& hoveringOnDropdown) {
    bool isUpdating = false;
    gui.push_id(id);
    auto& palette = paletteData.palettes[paletteData.selectedPalette].colors;
    size_t i = 0;
    constexpr float COLOR_BUTTON_SIZE = GUIStuff::GUIManager::BIG_BUTTON_SIZE;
    while(i < palette.size()) {
        CLAY({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(COLOR_BUTTON_SIZE)},
                .padding = {.top = 3, .bottom = 3},
                .childGap = io->theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            }
        }) {
            while(i < palette.size()) {
                CLAY({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIXED(COLOR_BUTTON_SIZE), .height = CLAY_SIZING_FIXED(COLOR_BUTTON_SIZE)},
                    }
                }) {
                    Vector4f newC = {palette[i].x(), palette[i].y(), palette[i].z(), 1.0f};
                    if(gui.color_button("c" + std::to_string(i), &newC, (paletteData.selectedColor == (int)i))) {
                        paletteData.selectedColor = (int)i;
                        // We want to keep the old color's alpha
                        color->x() = newC.x();
                        color->y() = newC.y();
                        color->z() = newC.z();
                        isUpdating = true;
                    }
                    if(paletteData.selectedColor == (int)i && (newC.x() != color->x() || newC.y() != color->y() || newC.z() != color->z()))
                        paletteData.selectedColor = -1;
                }
                i++;
                if(i % 6 == 0)
                    break;
            }
        }
    }
    if(paletteData.selectedPalette != 0) {
        CLAY({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                .padding = {.top = 3, .bottom = 3},
                .childGap = io->theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            }
        }) {
            if(gui.svg_icon_button("addcolor", "data/icons/plus.svg", false, COLOR_BUTTON_SIZE)) {
                palette.emplace_back(color->x(), color->y(), color->z());
                paletteData.selectedColor = palette.size() - 1;
            }
            if(gui.svg_icon_button("deletecolor", "data/icons/close.svg", false, COLOR_BUTTON_SIZE)) {
                if(paletteData.selectedColor >= 0 && paletteData.selectedColor < (int)palette.size()) {
                    palette.erase(palette.begin() + paletteData.selectedColor);
                    paletteData.selectedColor = -1;
                }
            }
        }
    }
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
            .padding = {.top = 3, .bottom = 3},
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        }
    }) {
        std::vector<std::string> paletteNames;
        for(auto& p : paletteData.palettes)
            paletteNames.emplace_back(p.name);
        gui.dropdown_select("paletteselector", &paletteData.selectedPalette, paletteNames, 200.0f, 90.0f, [&]() {
            hoveringOnDropdown = Clay_Hovered();
        });
        if(gui.svg_icon_button("paletteadd", "data/icons/plus.svg", paletteData.addingPalette, 25.0f)) paletteData.addingPalette = !paletteData.addingPalette;
        if(paletteData.selectedPalette != 0) {
            if(gui.svg_icon_button("paletteremove", "data/icons/close.svg", false, 25.0f)) {
                paletteData.palettes.erase(paletteData.palettes.begin() + paletteData.selectedPalette);
                paletteData.selectedPalette = 0;
            }
        }
    }
    if(paletteData.addingPalette) {
        gui.input_text_field("paletteinputname", "Name", &paletteData.newPaletteStr);
        if(gui.text_button_wide("addpalettebutton", "Create") && !paletteData.newPaletteStr.empty()) {
            paletteData.palettes.emplace_back();
            paletteData.palettes.back().name = paletteData.newPaletteStr;
            paletteData.selectedPalette = paletteData.palettes.size() - 1;
            paletteData.addingPalette = false;
        }
    }
    gui.pop_id();

    return isUpdating;
}

void Toolbar::performance_metrics() {
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
        .floating = {.offset = {-10, -10}, .attachPoints = {.element = CLAY_ATTACH_POINT_RIGHT_BOTTOM, .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        std::stringstream a;
        a << "FPS: " << std::fixed << std::setprecision(0) << (1.0 / main.deltaTime);
        gui.text_label(a.str());
        gui.text_label("Item Count: " + std::to_string(main.world->drawProg.components.client_list().size()));
        std::stringstream b;
        b << "Coord: " << main.world->drawData.cam.c.pos.x().display_int_str(5) << ", " << main.world->drawData.cam.c.pos.y().display_int_str(5);
        gui.text_label(b.str());
        std::stringstream c;
        c << "Zoom: " << main.world->drawData.cam.c.inverseScale.display_int_str(5);
        gui.text_label(c.str());
        std::stringstream d;
        d << "Rotation: " << main.world->drawData.cam.c.rotation;
        gui.text_label(d.str());
    }
}

void Toolbar::player_list() {
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(500), .height = CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
        .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        gui.obstructing_window();
        gui.push_id("client list");
        gui.text_label_centered("Player List");
        gui.left_to_right_line_layout([&]() {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)}
                },
                .backgroundColor = convert_vec4<Clay_Color>(SkColor4f{main.world->userColor.x(), main.world->userColor.y(), main.world->userColor.z(), 1.0f}),
                .cornerRadius = CLAY_CORNER_RADIUS(3)
            }) {}
            gui.text_label(main.world->displayName);
        });
        size_t num = 0;
        for(auto& [id, client] : main.world->clients) {
            gui.push_id(num++);
            gui.left_to_right_line_layout([&]() {
                CLAY({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)}
                    },
                    .backgroundColor = convert_vec4<Clay_Color>(SkColor4f{client.cursorColor.x(), client.cursorColor.y(), client.cursorColor.z(), 1.0f}),
                    .cornerRadius = CLAY_CORNER_RADIUS(3)
                }) {}
                gui.text_label(client.displayName);
                CLAY({.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
                if(gui.text_button("teleport button", "Jump To"))
                    main.world->drawData.cam.smooth_move_to(*main.world, client.camCoords, client.windowSize);
            });
            gui.pop_id();
        }
        if(gui.text_button_wide("close list", "Done"))
            playerMenuOpen = false;
        gui.pop_id();
    }
}

void Toolbar::open_world_file(int conType, const std::string& netSource, const std::string& serverLocalID2) {
#ifdef __EMSCRIPTEN__
    static struct UploadData {
        int cT;
        std::string nS;
        std::string sLID;
        MainProgram* main;
    } uploadData;
    uploadData.cT = conType;
    uploadData.nS = netSource;
    uploadData.sLID = serverLocalID2;
    uploadData.main = &main;
    emscripten_browser_file::upload("." + World::FILE_EXTENSION, [](std::string const& fileName, std::string const& mimeType, std::string_view buffer, void* callbackData) {
        if(!buffer.empty()) {
            UploadData* uD = (UploadData*)callbackData;
            uD->main->new_tab({
                .conType = (World::ConnectionType)uD->cT,
                .filePathSource = std::filesystem::path(fileName),
                .netSource = uD->nS,
                .serverLocalID = uD->sLID,
                .fileDataBuffer = buffer
            }, true);
        }
    }, &uploadData);
#else
    open_file_selector("Open", {{"Any File", "*"}, {"InfiniPaint Canvas", World::FILE_EXTENSION}}, [&, conType = conType, netSource = netSource, serverLocalID2 = serverLocalID2](const std::filesystem::path& p, const auto& e) {
        main.new_tab({
            .conType = (World::ConnectionType)conType,
            .filePathSource = p,
            .netSource = netSource,
            .serverLocalID = serverLocalID2
        });
    });
#endif
}

void Toolbar::options_menu() {
    switch(optionsMenuType) {
        case HOST_MENU:
        case CONNECT_MENU: {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(500), .height = CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                    .childGap = io->theme->childGap1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
                .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
            }) {
                gui.obstructing_window();
                if(optionsMenuType == HOST_MENU) {
                    gui.push_id("host menu");
                    std::string oldS = serverToConnectTo;
                    gui.input_text_field("lobby", "Lobby", &serverToConnectTo);
                    serverToConnectTo = oldS;
                    gui.left_to_right_line_layout([&]() {
                        if(gui.text_button_wide("copy lobby address", "Copy Lobby Address"))
                            main.input.set_clipboard_str(serverToConnectTo);
                        if(gui.text_button_wide("host file", "Host")) {
                            main.world->start_hosting(serverToConnectTo, serverLocalID);
                            optionsMenuOpen = false;
                        }
                        if(gui.text_button_wide("cancel", "Cancel"))
                            optionsMenuOpen = false;
                    });
                    gui.pop_id();
                }
                else if(optionsMenuType == CONNECT_MENU) {
                    gui.push_id("connect menu");
                    gui.input_text_field("lobby", "Lobby", &serverToConnectTo);
                    gui.left_to_right_line_layout([&]() {
                        if(gui.text_button_wide("connect", "Connect")) {
                            if(serverToConnectTo.length() != (NetLibrary::LOCALID_LEN + NetLibrary::GLOBALID_LEN))
                                Logger::get().log("USERINFO", "Connect issue: Incorrect address length");
                            else if(serverToConnectTo.substr(0, NetLibrary::GLOBALID_LEN) == NetLibrary::get_global_id())
                                Logger::get().log("USERINFO", "Connect issue: Can't connect to your own address");
                            else {
                                main.new_tab({
                                    .conType = World::CONNECTIONTYPE_CLIENT,
                                    .netSource = serverToConnectTo
                                }, true);
                                optionsMenuOpen = false;
                            }
                        }
                        if(gui.text_button_wide("cancel", "Cancel"))
                            optionsMenuOpen = false;
                    });
                    gui.pop_id();
                }
            }
            break;
        }
        case GENERAL_SETTINGS_MENU: {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIXED(600), .height = CLAY_SIZING_FIXED(500) },
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
                .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
            }) {
                gui.push_id("gsettings");
                CLAY({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                        .padding = CLAY_PADDING_ALL(io->theme->padding1),
                        .childGap = io->theme->childGap1,
                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                    }
                }) {
                    gui.obstructing_window();
                    CLAY({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIT(150), .height = CLAY_SIZING_GROW(0) },
                            .padding = CLAY_PADDING_ALL(io->theme->padding1),
                            .childGap = 2,
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        }
                    }) {
                        auto oldOption = generalSettingsOptions;
                        if(gui.text_button_left_transparent("Generalbutton", "General", generalSettingsOptions == GSETTINGS_GENERAL)) generalSettingsOptions = GSETTINGS_GENERAL;
                        if(gui.text_button_left_transparent("Tabletbutton", "Tablet", generalSettingsOptions == GSETTINGS_TABLET)) generalSettingsOptions = GSETTINGS_TABLET;
                        if(gui.text_button_left_transparent("Themebutton", "Theme", generalSettingsOptions == GSETTINGS_THEME)) generalSettingsOptions = GSETTINGS_THEME;
                        if(gui.text_button_left_transparent("Keybindsbutton", "Keybinds", generalSettingsOptions == GSETTINGS_KEYBINDS)) generalSettingsOptions = GSETTINGS_KEYBINDS;
                        if(gui.text_button_left_transparent("Debugbutton", "Debug", generalSettingsOptions == GSETTINGS_DEBUG)) generalSettingsOptions = GSETTINGS_DEBUG;
                        if(oldOption != generalSettingsOptions) {
                            load_theme();
                            themeData.selectedThemeIndex = std::nullopt;
                        }
                    }
                    CLAY({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        }
                    }) {
                        gui.scroll_bar_area("general settings scroll area", [&](float, float, float) {
                            CLAY({
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                                    .childGap = io->theme->childGap1,
                                    .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                                }
                            }) {
                                switch(generalSettingsOptions) {
                                    case GSETTINGS_GENERAL: {
                                        gui.push_id("general settings");
                                        gui.input_text_field("display name input", "Display Name", &main.displayName);
                                        main.update_display_names();
                                        SkColor4f newBackColor{main.defaultCanvasBackgroundColor.x(), main.defaultCanvasBackgroundColor.y(), main.defaultCanvasBackgroundColor.z(), 1.0f};
                                        gui.color_picker_button_field("defaultCanvasBackgroundColor", "Default canvas background color", &newBackColor, false);
                                        main.defaultCanvasBackgroundColor = convert_vec3<Vector3f>(newBackColor);
                                        #ifndef __EMSCRIPTEN__
                                            gui.checkbox_field("native file pick", "Use Native File Picker", &useNativeFilePicker);
                                            gui.checkbox_field("update notifications enable", "Check For Updates on Startup", &updateCheckerData.checkForUpdates);
                                        #endif
                                        gui.slider_scalar_field("drag zoom slider", "Drag zoom speed", &dragZoomSpeed, 0.0, 1.0, 3);
                                        gui.slider_scalar_field("scroll zoom slider", "Scroll zoom speed", &scrollZoomSpeed, 0.0, 1.0, 3);
                                        gui.checkbox_field("flip zoom tool direction", "Flip zoom tool direction", &flipZoomToolDirection);
                                        gui.input_scalar_field("jump transition time", "Jump transition time", &jumpTransitionTime, 0.01f, 1000.0f, 2);
                                        gui.checkbox_field("changebrushwidthwithspeed", "Change brush size with mouse speed", &velocityAffectsBrushWidth);
                                        gui.input_scalar_field("Max GUI Scale", "Max GUI Scale", &guiScale, 0.5f, 3.0f, 1);

                                        gui.text_label("VSync:");
                                        if(gui.radio_button_field("Vsync On", "On", main.window.vsyncValue == 1))
                                            main.set_vsync_value(1);
                                        if(gui.radio_button_field("Vsync Off", "Off", main.window.vsyncValue == 0))
                                            main.set_vsync_value(0);
                                        if(gui.radio_button_field("Vsync Adaptive", "Adaptive", main.window.vsyncValue == -1))
                                            main.set_vsync_value(-1);

                                        gui.pop_id();
                                        break;
                                    }
                                    case GSETTINGS_TABLET: {
                                        gui.push_id("tablet settings");
                                        gui.checkbox_field("pen pressure width", "Pen pressure affects brush size", &tabletOptions.pressureAffectsBrushWidth);
                                        gui.slider_scalar_field("smoothing time", "Smoothing sampling time", &tabletOptions.smoothingSamplingTime, 0.001f, 1.0f, 3);
                                        gui.input_scalar_field<uint8_t>("middle click", "Middle click pen button", &tabletOptions.middleClickButton, 1, 255);
                                        gui.input_scalar_field<uint8_t>("right click", "Right click pen button", &tabletOptions.rightClickButton, 1, 255);
                                        #ifdef _WIN32
                                            gui.checkbox_field("mouse ignore when pen proximity", "Ignore mouse movement when pen in proximity", &tabletOptions.ignoreMouseMovementWhenPenInProximity);
                                        #endif
                                        gui.pop_id();
                                        break;
                                    }
                                    case GSETTINGS_THEME: {
                                        gui.push_id("theme");
                                        if(!themeData.selectedThemeIndex)
                                            reload_theme_list();

                                        CLAY({.layout = { 
                                              .childGap = io->theme->childGap1,
                                              .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                                              .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                        }
                                        }) {
                                            gui.text_label("Theme: ");
                                            size_t changedVal = themeData.selectedThemeIndex.value();
                                            gui.dropdown_select("dropdownSelectThemes", &themeData.selectedThemeIndex.value(), themeData.themeDirList);
                                            if(changedVal != themeData.selectedThemeIndex.value()) {
                                                themeData.themeCurrentlyLoaded = themeData.themeDirList[themeData.selectedThemeIndex.value()];
                                                reload_theme_list();
                                            }
                                        }
                                        gui.left_to_right_line_layout([&]() {
                                            if(themeData.selectedThemeIndex != 0 && gui.text_button_wide("savethemebutton", "Save")) {
                                                themeData.themeCurrentlyLoaded = themeData.themeDirList[themeData.selectedThemeIndex.value()];
                                                save_theme();
                                                reload_theme_list();
                                            }
                                            if(gui.text_button_wide("saveasthemebutton", "Save As", themeData.openedSaveAsMenu)) themeData.openedSaveAsMenu = !themeData.openedSaveAsMenu;
                                            if(gui.text_button_wide("reloadthemebutton", "Reload")) {
                                                themeData.themeCurrentlyLoaded = themeData.themeDirList[themeData.selectedThemeIndex.value()];
                                                reload_theme_list();
                                            }
                                            if(themeData.selectedThemeIndex != 0 && gui.text_button_wide("deletethemebutton", "Delete")) {
                                                try { std::filesystem::remove(main.configPath / "themes" / (themeData.themeDirList[themeData.selectedThemeIndex.value()] + ".json")); } catch(...) { }
                                                themeData.themeCurrentlyLoaded = "Default";
                                                reload_theme_list();
                                            }
                                        });
                                        if(themeData.openedSaveAsMenu) {
                                            gui.left_to_right_line_layout([&]() {
                                                gui.text_label("Theme name: ");
                                                gui.input_text("saveasnewthemename", &themeData.themeCurrentlyLoaded);
                                            });
                                            gui.left_to_right_line_layout([&]() {
                                                if(gui.text_button_wide("saveasdone", "Done")) {
                                                    save_theme();
                                                    reload_theme_list();
                                                }
                                                if(gui.text_button_wide("saveascancel", "Cancel"))
                                                    themeData.openedSaveAsMenu = false;
                                            });
                                        }
                                        gui.text_label("Edit theme:");
                                        gui.text_label_light("Note: Changes only remain if theme is saved");
                                        gui.color_picker_button_field("fillColor1", "Fill Color 1", &io->theme->fillColor1, true);
                                        gui.color_picker_button_field("fillColor2", "Fill Color 2", &io->theme->fillColor2, true);
                                        gui.color_picker_button_field("backColor1", "Back Color 1", &io->theme->backColor1, true);
                                        gui.color_picker_button_field("backColor2", "Back Color 2", &io->theme->backColor2, true);
                                        gui.color_picker_button_field("frontColor1", "Front Color 1", &io->theme->frontColor1, true);
                                        gui.color_picker_button_field("frontColor2", "Front Color 2", &io->theme->frontColor2, true);
                                        gui.color_picker_button_field("warningColor", "Warning Color", &io->theme->warningColor, true);
                                        gui.color_picker_button_field("errorColor", "Error Color", &io->theme->errorColor, true);
                                        //gui.slider_scalar_field("hoverExpandTime", "Hover Expand Time", &io->theme->hoverExpandTime, 0.001f, 1.0f);
                                        gui.input_scalar_field<uint16_t>("childGap1", "Gap between child elements", &io->theme->childGap1, 0, 30);
                                        gui.input_scalar_field<uint16_t>("padding1", "Window padding", &io->theme->padding1, 0, 30);
                                        gui.slider_scalar_field<float>("windowCorners1", "Window corner radius", &io->theme->windowCorners1, 0, 30);
                                        gui.pop_id();
                                        break;
                                    }
                                    case GSETTINGS_KEYBINDS: {
                                        if(keybindWaiting.has_value()) {
                                            main.input.stop_key_input();
                                            if(main.input.lastPressedKeybind) {
                                                unsigned v = keybindWaiting.value();

                                                Vector2ui32 newKey = main.input.lastPressedKeybind.value();
                                                main.input.keyAssignments.erase(newKey);
                                                auto f = std::find_if(main.input.keyAssignments.begin(), main.input.keyAssignments.end(), [&](auto& p) {
                                                    return p.second == v;
                                                });
                                                if(f != main.input.keyAssignments.end())
                                                    main.input.keyAssignments.erase(f);
                                                main.input.keyAssignments.emplace(newKey, v);
                                                keybindWaiting = std::nullopt;
                                            }
                                        }

                                        gui.push_id("keybind entries");
                                        for(unsigned i = 0; i < InputManager::KEY_ASSIGNABLE_COUNT; i++) {
                                            gui.push_id(i);
                                            CLAY({
                                                .layout = {
                                                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                                    .padding = CLAY_PADDING_ALL(0),
                                                    .childGap = io->theme->childGap1,
                                                    .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                                                    .layoutDirection = CLAY_LEFT_TO_RIGHT 
                                                }
                                            }) {
                                                gui.text_label(nlohmann::json(static_cast<InputManager::KeyCodeEnum>(i)));
                                                auto f = std::find_if(main.input.keyAssignments.begin(), main.input.keyAssignments.end(), [&](auto& p) {
                                                    return p.second == i;
                                                });
                                                std::string assignedKeystrokeStr = f != main.input.keyAssignments.end() ? main.input.key_assignment_to_str(f->first) : "";
                                                if(gui.text_button_wide("keybind button", assignedKeystrokeStr, keybindWaiting.has_value() && keybindWaiting.value() == i))
                                                    keybindWaiting = i;
                                            }
                                            gui.pop_id();
                                        }
                                        gui.pop_id();
                                        break;
                                    }
                                    case GSETTINGS_DEBUG: {
                                        gui.push_id("debug settings menu");
                                        gui.checkbox_field("show performance metrics", "Show Metrics", &showPerformance);
                                        gui.input_scalar_fields("jump transition easing", "Jump Easing", &jumpTransitionEasing, 4, -10.0f, 10.0f, 2);
                                        #ifndef __EMSCRIPTEN__
                                            gui.input_scalar_field("fps cap slider", "FPS cap", &main.fpsLimit, 3.0f, 10000.0f);
                                        #endif
                                        gui.pop_id();
                                        break;
                                    }
                                }
                            }
                        });
                        if(gui.text_button_wide("done menu", "Done")) {
                            main.save_config();
                            load_theme();
                            themeData.selectedThemeIndex = std::nullopt;
                            optionsMenuOpen = false;
                        }
                    }
                }
                gui.pop_id();
            }
            break;
        }
        case LOBBY_INFO_MENU: {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(500), .height = CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                    .childGap = io->theme->childGap1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
                .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
            }) {
                gui.push_id("lobby info menu");
                gui.obstructing_window();
                if(!main.world || !main.world->network_being_used())
                    optionsMenuOpen = false;
                else {
                    std::string oldS = main.world->netSource;
                    gui.input_text_field("lobby", "Lobby", &main.world->netSource);
                    main.world->netSource = oldS;
                    gui.left_to_right_line_layout([&]() {
                        if(gui.text_button_wide("copy lobby address", "Copy Lobby Address"))
                            main.input.set_clipboard_str(main.world->netSource);
                        if(gui.text_button_wide("done", "Done"))
                            optionsMenuOpen = false;
                    });
                }
                gui.pop_id();
            }
            break;
        }
        case CANVAS_SETTINGS_MENU: {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(500), .height = CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                    .childGap = io->theme->childGap1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
                .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
            }) {
                gui.push_id("canvas settings menu");
                gui.obstructing_window();
                SkColor4f newColorToSet = main.world->canvasTheme.backColor;
                gui.color_picker_button_field("canvasColor", "Canvas Color", &newColorToSet, false);
                main.world->set_canvas_background_color(convert_vec3<Vector3f>(newColorToSet));
                if(gui.text_button_wide("done", "Done"))
                    optionsMenuOpen = false;
                gui.pop_id();
            }
            break;
        }
        case SET_DOWNLOAD_NAME: {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(500), .height = CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                    .childGap = io->theme->childGap1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
                .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
            }) {
                gui.push_id("set download name menu");
                gui.obstructing_window();
                if(!main.world)
                    optionsMenuOpen = false;
                gui.input_text_field("file name", "File Name", &downloadNameSet);
                gui.left_to_right_line_layout([&]() {
                    if(gui.text_button_wide("download save button", "Save")) {
                        main.world->save_to_file(downloadNameSet);
                        optionsMenuOpen = false;
                    }
                    if(gui.text_button_wide("cancel", "Cancel"))
                        optionsMenuOpen = false;
                });
                gui.pop_id();
            }
            break;
        }
        case ABOUT_MENU: {
            about_menu_gui();
            break;
        }
    }
}

void Toolbar::about_menu_gui() {
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(700), .height = CLAY_SIZING_FIXED(600) },
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
        .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        gui.push_id("about menu popup");
        gui.obstructing_window();
        gui.left_to_right_line_layout([&]() {
            gui.push_id("Menu Selector");
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(200), .height = CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(io->theme->padding1),
                    .childGap = 1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                }
            }) {
                gui.scroll_bar_area("About Menu Selector Scroll Area", [&](float, float, float) {
                    CLAY({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                            .padding = CLAY_PADDING_ALL(0),
                            .childGap = 1,
                        }
                    }) {
                        if(gui.text_button_left_transparent("infinipaintnoticebutton", "InfiniPaint", selectedLicense == -1)) selectedLicense = -1;
                    }
                    gui.text_label_light_centered("Third Party Components");
                    for(int i = 0; i < static_cast<int>(thirdPartyLicenses.size()); i++) {
                        CLAY({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                .padding = CLAY_PADDING_ALL(0),
                                .childGap = 1,
                            }
                        }) {
                            gui.push_id(i);
                            if(gui.text_button_left_transparent("noticebutton", thirdPartyLicenses[i].first, selectedLicense == i)) selectedLicense = i;
                            gui.pop_id();
                        }
                    }
                });
            }
            gui.pop_id();
            gui.scroll_bar_area("About Menu Text Scroll Area", [&](float, float, float) {
                gui.text_label_size((selectedLicense == -1) ? ownLicenseText : thirdPartyLicenses[selectedLicense].second, 0.8f);
            });
        });
        if(gui.text_button_wide("done", "Done"))
            optionsMenuOpen = false;
        gui.pop_id();
    }
}

void Toolbar::reload_theme_list() {
    themeData.selectedThemeIndex = 0;

    themeData.themeDirList.clear();
    themeData.themeDirList.emplace_back("Default");
    std::filesystem::path themeDir = main.configPath / "themes";
    if(std::filesystem::exists(themeDir) && std::filesystem::is_directory(themeDir)) {
        for(auto& theme : std::filesystem::recursive_directory_iterator(themeDir)) {
            std::string name = theme.path().stem().string();
            if(name != "Default") {
                themeData.themeDirList.emplace_back(name);
                if(name == themeData.themeCurrentlyLoaded)
                    themeData.selectedThemeIndex = themeData.themeDirList.size() - 1;
            }
        }
    }
    if(!load_theme())
        themeData.selectedThemeIndex = 0;

    themeData.openedSaveAsMenu = false;
}

void Toolbar::file_picker_gui() {
    gui.push_id("filepicker");
    bool isDoneByDoubleClick = false;
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(700), .height = CLAY_SIZING_FIXED(500) },
            .padding = CLAY_PADDING_ALL(io->theme->padding1),
            .childGap = io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io->theme->windowCorners1),
        .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        gui.obstructing_window();
        gui.text_label_centered(filePicker.filePickerWindowName);
        gui.left_to_right_line_layout([&]() {
            if(gui.svg_icon_button("file picker back button", "data/icons/backarrow.svg", false, 30.0f)) {
                filePicker.currentSearchPath = filePicker.currentSearchPath.parent_path();
                filePicker.refreshEntries = true;
            }
            std::filesystem::path pathDiff = filePicker.currentSearchPath;
            gui.input_path_field("file picker path", "Path", &filePicker.currentSearchPath, std::filesystem::file_type::directory);
            if(pathDiff != filePicker.currentSearchPath)
                filePicker.refreshEntries = true;
        });
        CLAY({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = convert_vec4<Clay_Color>(io->theme->backColor2)
        }) {
            if(filePicker.refreshEntries) {
                filePicker.entries.clear();
                for(;;) {
                    try {
                        for(const std::filesystem::path& entry : std::filesystem::directory_iterator(filePicker.currentSearchPath))
                            filePicker.entries.emplace_back(entry);
                        break;
                    }
                    catch(const std::exception& e) {
                        Logger::get().log("INFO", e.what());
                        if(filePicker.currentSearchPath == main.homePath) // The home path must exist. If we get errors on the home path, we have a real problem
                            throw e;
                        filePicker.currentSearchPath = main.homePath;
                    }
                }

                std::vector<std::string> extensionList = split_string_by_token(filePicker.extensionFilters[filePicker.extensionSelected], ";");
                for(std::string& s : extensionList) {
                    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                    s.insert(0, ".");
                }

                std::erase_if(filePicker.entries, [&](const std::filesystem::path& a) {
                    if(extensionList.empty())
                        return false;
                    if(extensionList[0] == ".*")
                        return false;
                    std::string fExtension;
                    if(a.has_extension()) {
                        fExtension = a.extension().string();
                        std::transform(fExtension.begin(), fExtension.end(), fExtension.begin(), ::tolower);
                    }
                    return std::filesystem::is_regular_file(a) && (fExtension.empty() || !std::ranges::contains(extensionList, fExtension));
                });

                std::sort(filePicker.entries.begin(), filePicker.entries.end(), [&](const std::filesystem::path& a, const std::filesystem::path& b) {
                    bool aDir = std::filesystem::is_directory(a);
                    bool bDir = std::filesystem::is_directory(b);
                    const std::string& aStr = a.string();
                    const std::string& bStr = b.string();
                    if(aDir && !bDir)
                        return true;
                    if(!aDir && bDir)
                        return false;
                    return std::lexicographical_compare(aStr.begin(), aStr.end(), bStr.begin(), bStr.end());
                });
                filePicker.refreshEntries = false;
            }
            float entryHeight = 25.0f;
            gui.scroll_bar_many_entries_area("file picker entries", entryHeight, filePicker.entries.size(), [&](size_t i, bool isListHovered) {
                const std::filesystem::path& entry = filePicker.entries[i];
                bool selectedEntry = filePicker.currentSelectedPath == entry;
                CLAY({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(entryHeight)},
                        .childGap = 1,
                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT 
                    },
                    .backgroundColor = selectedEntry ? convert_vec4<Clay_Color>(io->theme->backColor1) : convert_vec4<Clay_Color>(io->theme->backColor2)
                }) {
                    CLAY({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)}
                        },
                    }) {
                        if(std::filesystem::is_directory(entry))
                            gui.svg_icon("folder icon", "data/icons/folder.svg", selectedEntry);
                        else
                            gui.svg_icon("file icon", "data/icons/file.svg", selectedEntry);
                    }
                    gui.text_label(entry.filename().string());
                    if(Clay_Hovered() && io->mouse.leftClick && isListHovered) {
                        if(selectedEntry && io->mouse.leftClick >= 2) {
                            if(std::filesystem::is_directory(entry)) {
                                filePicker.currentSearchPath = entry;
                                filePicker.refreshEntries = true;
                            }
                            else if(std::filesystem::is_regular_file(entry))
                                isDoneByDoubleClick = true;
                        }
                        else {
                            filePicker.currentSelectedPath = entry;
                            if(std::filesystem::is_regular_file(entry))
                                filePicker.fileName = entry.filename().string();
                        }
                    }
                }
            });
        }
        gui.left_to_right_line_layout([&]() {
            gui.input_text("filepicker filename", &filePicker.fileName);
            size_t oldExtensionSelected = filePicker.extensionSelected;
            gui.dropdown_select("filepicker select type", &filePicker.extensionSelected, filePicker.extensionFilters);
            if(oldExtensionSelected != filePicker.extensionSelected)
                filePicker.refreshEntries = true;
        });
        gui.left_to_right_line_layout([&]() {
            std::filesystem::path pathToRet = std::filesystem::path();
            if(gui.text_button_wide("filepicker done", "Done") || isDoneByDoubleClick) {
                if(!filePicker.fileName.empty()) {
                    pathToRet = filePicker.currentSearchPath / filePicker.fileName;
                    filePicker.postSelectionFunc(pathToRet, filePicker.extensionFiltersComplete[filePicker.extensionSelected]);
                }
                filePicker.isOpen = false;
            }
            if(gui.text_button_wide("filepicker cancel", "Cancel")) {
                filePicker.isOpen = false;
            }
        });
    }
    gui.pop_id();
}


void Toolbar::initialize_io_before_update() {
    io->mouse.leftClick = main.input.mouse.leftClicks;
    io->mouse.rightClick = main.input.mouse.rightClicks;
    io->mouse.leftHeld = main.input.mouse.leftDown;
    io->mouse.rightHeld = main.input.mouse.rightDown;
    io->mouse.globalPos = main.input.mouse.pos / final_gui_scale();
    io->mouse.scroll = main.input.mouse.scrollAmount;
    io->deltaTime = main.deltaTime;

    io->key.left = main.input.key(InputManager::KEY_TEXT_LEFT).repeat;
    io->key.right = main.input.key(InputManager::KEY_TEXT_RIGHT).repeat;
    io->key.up = main.input.key(InputManager::KEY_TEXT_UP).repeat;
    io->key.down = main.input.key(InputManager::KEY_TEXT_DOWN).repeat;
    io->key.leftShift = main.input.key(InputManager::KEY_TEXT_SHIFT).held;
    io->key.leftCtrl = main.input.key(InputManager::KEY_TEXT_CTRL).held;
    io->key.home = main.input.key(InputManager::KEY_TEXT_HOME).repeat;
    io->key.del = main.input.key(InputManager::KEY_TEXT_DELETE).repeat;
    io->key.backspace = main.input.key(InputManager::KEY_TEXT_BACKSPACE).repeat;
    io->key.enter = main.input.key(InputManager::KEY_TEXT_ENTER).repeat;
    io->key.selectAll = main.input.key(InputManager::KEY_TEXT_SELECTALL).repeat;
    io->key.copy = main.input.key(InputManager::KEY_TEXT_COPY).repeat;
    io->key.paste = main.input.get_clipboard_paste_happened();
    io->key.cut = main.input.key(InputManager::KEY_TEXT_CUT).repeat;
    io->key.escape = main.input.key(InputManager::KEY_GENERIC_ESCAPE).repeat;
    io->richTextBoxEdit = [&](std::shared_ptr<RichText::TextBox> t, std::shared_ptr<RichText::TextBox::Cursor> c) {
        main.input.text.set_rich_text_box_input(t, c);
    };

    if(io->acceptingTextInput)
        main.input.text_input_silence_everything();
    io->acceptingTextInput = false;
}

void Toolbar::start_gui() {
    gui.windowPos = Vector2f{0.0f, 0.0f};
    gui.windowSize = main.window.size.cast<float>() / final_gui_scale();
    gui.io = io;
    io->hoverObstructed = false;
    gui.begin();
}

float Toolbar::final_gui_scale() {
    return finalCalculatedGuiScale;
}

void Toolbar::calculate_final_gui_scale() {
    Vector2f maxWindowSizeBeforeForcedFit = final_gui_scale_not_fit() * Vector2f{700.0f, 700.0f};
    Vector2f fitRatio = {main.window.size.x() / maxWindowSizeBeforeForcedFit.x(), main.window.size.y() / maxWindowSizeBeforeForcedFit.y()};
    finalCalculatedGuiScale = final_gui_scale_not_fit() * std::min(std::min(fitRatio.x(), fitRatio.y()), 1.0f);
}

float Toolbar::final_gui_scale_not_fit() {
    return guiScale * main.window.scale;
}

void Toolbar::end_gui() {
    gui.end();
    if(io->clipboard.textOut)
        main.input.set_clipboard_str(*io->clipboard.textOut);
}

void Toolbar::draw(SkCanvas* canvas) {
    if(!main.takingScreenshot) {
        canvas->save();
        canvas->scale(final_gui_scale(), final_gui_scale());
        gui.draw(canvas);
        canvas->restore();
    }
}
