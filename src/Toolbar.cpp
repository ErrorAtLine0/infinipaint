#include "Toolbar.hpp"
#include "DrawingProgram/Tools/DrawingProgramToolBase.hpp"
#include "DrawingProgram/Tools/ScreenshotTool.hpp"
#include "FileHelpers.hpp"
#include "GUIStuff/ElementHelpers/CheckBoxHelpers.hpp"
#include "GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "GUIStuff/ElementHelpers/PopupHelpers.hpp"
#include "GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"
#include "GUIStuff/ElementHelpers/ScrollAreaHelpers.hpp"
#include "Helpers/ConvertVec.hpp"
#include "Helpers/FileDownloader.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "Helpers/NetworkingObjects/NetObjGenericSerializedClass.hpp"
#include "MainProgram.hpp"
#include "InputManager.hpp"
#include "ResourceDisplay/ImageResourceDisplay.hpp"
#include "VersionConstants.hpp"
#include "World.hpp"
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_filesystem.h>
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

#include "GUIStuff/GUIManager.hpp"
#include "GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "GUIStuff/ElementHelpers/PopupHelpers.hpp"
#include "GUIStuff/ElementHelpers/TextBoxHelpers.hpp"
#include "GUIStuff/ElementHelpers/ColorPickerHelpers.hpp"
#include "GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"
#include "GUIStuff/ElementHelpers/NumberSliderHelpers.hpp"
#include "GUIStuff/Elements/ScrollArea.hpp"
#include "GUIStuff/Elements/LayoutElement.hpp"
#include "GUIStuff/Elements/DropDown.hpp"
#include "GUIStuff/Elements/SVGIcon.hpp"

#define UPDATE_DOWNLOAD_URL "https://infinipaint.com/download.html"
#define UPDATE_NOTIFICATION_URL "https://infinipaint.com/updateNotificationVersion.txt"

#ifdef __EMSCRIPTEN__
    #include <EmscriptenHelpers/emscripten_browser_file.h>
#endif

Toolbar::NativeFilePicker Toolbar::nativeFilePicker;

using namespace GUIStuff;
using namespace ElementHelpers;

Toolbar::Toolbar(MainProgram& initMain):
    main(initMain)
{
    load_default_palette();
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
    if(main.conf.useNativeFilePicker) {
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
        filePicker.extensionFiltersComplete = extensionFilters;
        filePicker.extensionFilters.clear();
        for(auto& [name, exList] : extensionFilters)
            filePicker.extensionFilters.emplace_back(exList);
        filePicker.extensionSelected = extensionFilters.size() - 1;
        filePicker.filePickerWindowName = filePickerName;
        filePicker.postSelectionFunc = postSelectionFunc;
        filePicker.fileName = "";
        filePicker.isSaving = isSaving;
        file_picker_gui_refresh_entries();
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
    {
        int globCount;
        std::filesystem::path third_party_license_path("data/third_party_licenses");
        char** filesInPath = SDL_GlobDirectory(third_party_license_path.c_str(), nullptr, 0, &globCount);
        if(filesInPath) {
            for(int i = 0; i < globCount; i++) {
                std::filesystem::path filePath = third_party_license_path / std::filesystem::path(filesInPath[i]);
                SDL_PathInfo fileInfo;
                if(SDL_GetPathInfo(filePath.c_str(), &fileInfo) && fileInfo.type == SDL_PATHTYPE_FILE) {
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

void Toolbar::layout_run() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

#ifndef __EMSCRIPTEN__
    update_notification_check();
#endif

    if(main.drawGui) {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIT(gui.io.windowSize.x()), .height = CLAY_SIZING_FIT(gui.io.windowSize.y())},
                .padding = CLAY_PADDING_ALL(io.theme->padding1),
                .childGap = io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }
        }) {
            top_toolbar();
            if(!main.world->clientStillConnecting)
                drawing_program_gui();
            if(!closePopupData.worldsToClose.empty())
                close_popup_gui();
            if(main.conf.viewWebVersionWelcome)
                web_version_welcome();
#ifndef __EMSCRIPTEN__
            else if(updateCheckerData.showGui)
                update_notification_gui();
#endif
            else if(filePicker.isOpen)
                file_picker_gui();
            else if(optionsMenuOpen)
                options_menu();
            else if(main.world->clientStillConnecting)
                still_connecting_center_message();
            else if(playerMenuOpen)
                player_list();
            else if(!main.world->drawProg.layerMan.is_a_layer_being_edited())
                no_layers_being_edited_message();

            if(showPerformance)
                performance_metrics();
        }
        if(!main.world->clientStillConnecting)
            chat_box();
    }
    else {
        if(!closePopupData.worldsToClose.empty()) // Should still show close popup if gui is disabled
            close_popup_gui();
    }

    if(!main.world->clientStillConnecting) {
        //if(io.hoverObstructed && (io.mouse.leftClick || io.mouse.rightClick))
        //    rightClickPopupLocation = std::nullopt;

        //if(rightClickPopupLocation.has_value()) {
        //    if(!main.world->drawProg.right_click_popup_gui(rightClickPopupLocation.value()))
        //        rightClickPopupLocation = std::nullopt;
        //}

        //if(io.hoverObstructed && io.mouse.rightClick) // This runs specifically if the paint popup has the cursor
        //    rightClickPopupLocation = std::nullopt;
    }

    justAssignedColorLeft = false;
    justAssignedColorRight = false;

    if(!optionsMenuOpen || generalSettingsOptions != GSETTINGS_KEYBINDS)
        keybindWaiting = std::nullopt;
}

bool Toolbar::app_close_requested() {
    for(auto& w : main.worlds) {
        if(w->should_ask_before_closing())
            add_world_to_close_popup_data(w);
    }
    closePopupData.closeAppWhenDone = true;
    return closePopupData.worldsToClose.empty();
}

void Toolbar::add_world_to_close_popup_data(const std::shared_ptr<World>& w) {
    auto it = std::find_if(closePopupData.worldsToClose.begin(), closePopupData.worldsToClose.end(), [&](auto& wStruct) {
        return wStruct.w.lock() == w;
    });
    if(it == closePopupData.worldsToClose.end())
        closePopupData.worldsToClose.emplace_back(w, true);
}

void Toolbar::close_popup_gui() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    std::erase_if(closePopupData.worldsToClose, [](auto& wPair) {
        return wPair.w.expired();
    });
    center_obstructing_window_gui("Close program popup GUI", CLAY_SIZING_FIT(0), CLAY_SIZING_FIT(0, 600), [&] {
        text_label(gui, "Files may contain unsaved changes");
        gui.clipping_element<ScrollArea>("close file popup gui scroll area", ScrollArea::Options{
            .scrollVertical = true,
            .clipVertical = true,
            .showScrollbarY = true,
            .innerContent = [&](const ScrollArea::InnerContentParameters&) {
                CLAY_AUTO_ID({
                    .layout = {
                        .childGap = io.theme->childGap1,
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM
                    }
                }) {
                    size_t i = 0;
                    for(auto& [w, setToSave] : closePopupData.worldsToClose) {
                        auto wLock = w.lock();
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                .padding = CLAY_PADDING_ALL(io.theme->padding1),
                                .childGap = io.theme->childGap1,
                                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT
                            },
                            .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor2),
                            .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1),
                        }) {
                            gui.new_id(i, [&] {
                                checkbox_boolean(gui, "set to save checkbox", &setToSave);
                                CLAY_AUTO_ID({
                                    .layout = {
                                        .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) },
                                        .childGap = 0,
                                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                                        .layoutDirection = CLAY_TOP_TO_BOTTOM
                                    }
                                }) {
                                    text_label(gui, wLock->name);
                                    text_label_light(gui, wLock->filePath.empty() ? "Autosave in " + main.documentsPath.string() : wLock->filePath.string());
                                }
                            });
                        }
                        ++i;
                    }
                }
            }
        });
        text_button(gui, "Save", "Save", {
            .wide = true,
            .onClick = [&] {
                for(auto& [w, setToSave] : closePopupData.worldsToClose) {
                    auto wLock = w.lock();
                    if(setToSave) {
                        if(wLock->filePath.empty())
                            wLock->autosave_to_directory(main.documentsPath);
                        else
                            wLock->save_to_file(wLock->filePath);
                    }
                    main.set_tab_to_close(w);
                }
                closePopupData.worldsToClose.clear();
                if(closePopupData.closeAppWhenDone)
                    main.setToQuit = true;
            }
        });
        text_button(gui, "Discard All", "Discard All", {
            .wide = true,
            .onClick = [&] {
                for(auto& [w, setToSave] : closePopupData.worldsToClose)
                    main.set_tab_to_close(w);
                closePopupData.worldsToClose.clear();
                if(closePopupData.closeAppWhenDone)
                    main.setToQuit = true;
            }
        });
        text_button(gui, "Cancel", "Cancel", {
            .wide = true,
            .onClick = [&] {
                closePopupData.worldsToClose.clear();
                closePopupData.closeAppWhenDone = false;
            }
        });
    });
}

void Toolbar::save_func() {
    if(main.world->filePath == std::filesystem::path())
        save_as_func();
    else
        main.world->save_to_file(main.world->filePath);
}

void Toolbar::save_as_func() {
    #ifdef __EMSCRIPTEN__
        optionsMenuOpen = true;
        optionsMenuType = SET_DOWNLOAD_NAME;
    #else
        open_file_selector("Save", {{"InfiniPaint Canvas", World::FILE_EXTENSION}}, [w = make_weak_ptr(main.world)](const std::filesystem::path& p, const auto& e) {
            auto world = w.lock();
            if(world)
                world->save_to_file(p);
        }, "", true);
    #endif
}

void Toolbar::paint_popup(Vector2f popupPos) {
    auto& gui = main.g.gui;
    auto& io = main.g.gui.io;

    double newRotationAngle = 0.0;
    paint_circle_popup_menu(gui, "paint circle popup", popupPos, {
        .rotationAngle = &main.world->drawData.cam.c.rotation,
        .selectedColor = main.world->drawProg.get_foreground_color_ptr(),
        .palette = paletteData.palettes[paletteData.selectedPalette].colors,
        .onRotate = [&] {
            main.world->drawData.cam.c.rotate_about(main.world->drawData.cam.c.from_space(main.window.size.cast<float>() * 0.5f), newRotationAngle - main.world->drawData.cam.c.rotation);
        },
        .onPaletteClick = [&] {
            gui.set_to_layout();
        }
    });
}

void Toolbar::top_toolbar() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    gui.element<LayoutElement>("top menu bar", [&] (const Clay_ElementId& lId) {
        CLAY(lId, {
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(static_cast<uint16_t>(io.theme->padding1 / 2)),
                .childGap = static_cast<uint16_t>(io.theme->childGap1 / 2),
                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            },
            .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
            .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1)
        }) {
            global_log();
            bool bookmarkMenuPopUpJustOpen = false;
            bool gridMenuPopUpJustOpen = false;
            bool layerMenuPopUpJustOpen = false;

            auto icon_button_top_toolbar = [&](const char* id, const std::string& svgPath, bool isSelected, const std::function<void()>& onClick) {
                svg_icon_button(gui, id, svgPath, {
                    .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                    .isSelected = isSelected,
                    .onClick = onClick
                });
            };

            menuPopupOpenFlipped = false;
            icon_button_top_toolbar("Main Menu Button", "data/icons/menu.svg", menuPopUpOpen, [&] {
                if(!menuPopupOpenFlipped) {
                    menuPopUpOpen = !menuPopUpOpen;
                    menuPopupOpenFlipped = false;
                }
            });


            // Temporary gap between buttons while tab list is being fixed
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                }
            }) { }


            //std::vector<std::pair<std::string, std::string>> tabNames;
            //for(size_t i = 0; i < main.worlds.size(); i++) {
            //    auto& w = main.worlds[i];
            //    bool shouldAddStarNextToName = w->should_ask_before_closing() && !w->netServer && !w->netClient;
            //    tabNames.emplace_back(w->netObjMan.is_connected() ? "data/icons/network.svg" : "", w->name + (shouldAddStarNextToName ? "*" : ""));
            //}

            //std::optional<size_t> closedTab;
            //gui.tab_list("file tab list", tabNames, main.worldIndex, closedTab);
            //if(closedTab) {
            //    auto& closedWorldPtr = main.worlds[closedTab.value()];
            //    if(closedWorldPtr->should_ask_before_closing())
            //        add_world_to_close_popup_data(closedWorldPtr);
            //    else
            //        main.set_tab_to_close(closedWorldPtr);
            //}

            if(!main.world->clientStillConnecting) {
                if(main.world->netObjMan.is_connected()) {
                    icon_button_top_toolbar("Player List Toggle Button", "data/icons/list.svg", playerMenuOpen, [&] {
                        playerMenuOpen = !playerMenuOpen;
                    });
                }
                icon_button_top_toolbar("Menu Undo Button", "data/icons/undo.svg", false, [&] {
                    main.world->undo_with_checks();
                });
                icon_button_top_toolbar("Menu Redo Button", "data/icons/redo.svg", false, [&] {
                    main.world->redo_with_checks();
                });
                icon_button_top_toolbar("Grids Button", "data/icons/grid.svg", gridMenu.popupOpen, [&] {
                    if(gridMenu.popupOpen)
                        stop_displaying_grid_menu();
                    else {
                        gridMenu.popupOpen = true;
                        gridMenuPopUpJustOpen = true;
                    }
                });
                icon_button_top_toolbar("Layer Menu Button", "data/icons/layer.svg", layerMenuPopupOpen, [&] {
                    if(layerMenuPopupOpen) {
                        main.world->drawProg.layerMan.listGUI.refresh_gui_data();
                        layerMenuPopupOpen = false;
                    }
                    else {
                        layerMenuPopupOpen = true;
                        layerMenuPopUpJustOpen = true;
                    }
                });
                icon_button_top_toolbar("Bookmark Menu Button", "data/icons/bookmark.svg", bookmarkMenuPopupOpen, [&] {
                    if(bookmarkMenuPopupOpen) {
                        main.world->bMan.refresh_gui_data();
                        bookmarkMenuPopupOpen = false;
                    }
                    else {
                        bookmarkMenuPopupOpen = true;
                        bookmarkMenuPopUpJustOpen = true;
                    }
                });

                if(gridMenu.popupOpen)
                    grid_menu(gridMenuPopUpJustOpen);
                if(bookmarkMenuPopupOpen)
                    bookmark_menu(bookmarkMenuPopUpJustOpen);
                if(layerMenuPopupOpen)
                    layer_menu(layerMenuPopUpJustOpen);
            }
            if(menuPopUpOpen) {
                gui.element<LayoutElement>("main menu popup", [&] (const Clay_ElementId& id) {
                    CLAY(id, {
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIT(100), .height = CLAY_SIZING_FIT(0) },
                            .padding = CLAY_PADDING_ALL(io.theme->padding1),
                            .childGap = 1,
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        },
                        .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
                        .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1),
                        .floating = {.offset = {.x = 0, .y = static_cast<float>(io.theme->padding1)}, .attachPoints = {.element = CLAY_ATTACH_POINT_LEFT_TOP, .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
                    }) {
                        auto menu_popup_text_button = [&](const char* id, const char* str, const std::function<void()>& onClick) {
                            text_button(gui, id, str, {
                                .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                .wide = true,
                                .centered = false,
                                .onClick = [&, onClick]{
                                    onClick();
                                    menuPopUpOpen = false;
                                }
                            });
                        };
                        menu_popup_text_button("new file local", "New File", [&] {
                            main.new_tab({
                                .isClient = false
                            }, true);
                        });
                        menu_popup_text_button("open file", "Open", [&] { open_world_file(false, "", ""); });
                        if(!main.world->clientStillConnecting) {
                            menu_popup_text_button("save file", "Save", [&] { save_func(); });
                            menu_popup_text_button("save as file", "Save As", [&] { save_as_func(); });
                            menu_popup_text_button("screenshot", "Screenshot", [&] { main.world->drawProg.switch_to_tool(DrawingProgramToolType::SCREENSHOT); });
                            menu_popup_text_button("add image or file to canvas", "Add Image/File to Canvas", [&] {
                                #ifdef __EMSCRIPTEN__
                                    static std::weak_ptr<World> worldWeakPtr;
                                    worldWeakPtr = make_weak_ptr(main.world);
                                    emscripten_browser_file::upload("*", [](std::string const& fileName, std::string const& mimeType, std::string_view buffer, void* callbackData) {
                                        if(!buffer.empty()) {
                                            auto world = ((std::weak_ptr<World>*)callbackData)->lock();
                                            if(world)
                                                world->drawProg.add_file_to_canvas_by_data(fileName, buffer, world->main.window.size.cast<float>() / 2.0f);
                                            else
                                                Logger::get().log("INFO", "Loading image to canvas that has been destroyed");
                                        }
                                    }, &worldWeakPtr);
                                #else
                                    open_file_selector("Open File", {{"Any File", "*"}}, [w = make_weak_ptr(main.world)](const std::filesystem::path& p, const auto& e) {
                                        auto wLock = w.lock();
                                        if(wLock)
                                            wLock->drawProg.add_file_to_canvas_by_path(p, wLock->main.window.size.cast<float>() / 2.0f, false);
                                        else
                                            Logger::get().log("INFO", "Loading image to canvas that has been destroyed");
                                    });
                                #endif
                            });
                            if(main.world->netObjMan.is_connected()) {
                                menu_popup_text_button("lobby info", "Lobby Info", [&] {
                                    optionsMenuOpen = true;
                                    optionsMenuType = LOBBY_INFO_MENU;
                                });
                            }
                            menu_popup_text_button("start hosting", "Host", [&] {
                                serverLocalID = NetLibrary::get_random_server_local_id();
                                serverToConnectTo = NetLibrary::get_global_id() + serverLocalID;
                                optionsMenuOpen = true;
                                optionsMenuType = HOST_MENU;
                            });
                            menu_popup_text_button("canvas specific settings", "Canvas Settings", [&] {
                                optionsMenuOpen = true;
                                optionsMenuType = CANVAS_SETTINGS_MENU;
                            });
                        }
                        menu_popup_text_button("start connecting", "Connect", [&] {
                            serverToConnectTo.clear();
                            optionsMenuOpen = true;
                            optionsMenuType = CONNECT_MENU;
                        });
                        menu_popup_text_button("open options", "Settings", [&] {
                            optionsMenuOpen = true;
                            optionsMenuType = GENERAL_SETTINGS_MENU;
                        });
                        menu_popup_text_button("about menu button", "About", [&] {
                            optionsMenuOpen = true;
                            optionsMenuType = ABOUT_MENU;
                        });
                        #ifndef __EMSCRIPTEN__
                            menu_popup_text_button("quit button", "Quit", [&] {
                                if(main.app_close_requested())
                                    main.setToQuit = true;
                            });
                        #endif
                    }
                }, LayoutElement::Callbacks {
                    .mouseButton = [&](const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) {
                        if(!mouseHovering && button.down && !menuPopupOpenFlipped) {
                            menuPopUpOpen = false;
                            menuPopupOpenFlipped = true;
                            gui.set_to_layout();
                        }
                        return mouseHovering;
                    }
                });
            }
        }
    });
}

void Toolbar::web_version_welcome() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(700), .height = CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(io.theme->padding1),
            .childGap = io.theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1),
        .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        gui.new_id("web version welcome gui", [&] {
            text_label_centered(gui, "Welcome to the web version of InfiniPaint!");
            text_label(gui, 
R"(This version contains more known issues than the native version of the app. This includes:
- Rare crashes
- If this browser tab is unfocused, or the window is minimized, any InfiniPaint tabs connected online (whether host or client) will be disconnected
- 4GB memory limit. Might be a problem if you're uploading many files/images
- Not multithreaded
- Can't access local fonts

If you like this app, consider downloading the native version for your system)");
            text_button(gui, "got it", "Got It", {
                .wide = true,
                .onClick = [&] {
                    main.conf.viewWebVersionWelcome = false;
                }
            });
        });
    }
}

void Toolbar::still_connecting_center_message() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(io.theme->padding1),
            .childGap = io.theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1),
        .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        text_label(gui, "Connecting to server...");
    }
}

void Toolbar::no_layers_being_edited_message() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(io.theme->padding1),
            .childGap = io.theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1),
        .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        text_label(gui, "Select a layer to edit");
    }
}

#ifndef __EMSCRIPTEN__
void Toolbar::update_notification_check() {
    if(!updateCheckerData.updateCheckDone) {
        if(main.conf.checkForUpdates) {
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
                            if(newV > currentV) {
                                updateCheckerData.showGui = true;
                                main.g.gui.set_to_layout();
                            }
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
    auto& gui = main.g.gui;

    center_obstructing_window_gui("Update notifications GUI", CLAY_SIZING_FIXED(700), CLAY_SIZING_FIT(0), [&] {
        gui.new_id("update notification gui", [&] {
            text_label_centered(gui, "Update v" + updateCheckerData.newVersionStr + " available!");
            text_button(gui, "download", "Open download page in web browser", {
                .wide = true,
                .onClick = [&]{
                    SDL_OpenURL(UPDATE_DOWNLOAD_URL);
                    updateCheckerData.showGui = false;
                }
            });
            text_button(gui, "ignore forever", "Ignore and don't notify again (can be changed in settings)", {
                .wide = true,
                .onClick = [&]{
                    main.conf.checkForUpdates = false;
                    updateCheckerData.showGui = false;
                }
            });
            text_button(gui, "ignore for now", "Ignore for now", {
                .wide = true,
                .onClick = [&]{
                    updateCheckerData.showGui = false;
                }
            });
        });
    });
}
#endif

void Toolbar::grid_menu(bool justOpened) {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    if(main.world->gridMan.grids) {
        gui.new_id("grid menu", [&] {
        //CLAY(localId, {
        //    .layout = {
        //        .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0, 600) },
        //        .padding = CLAY_PADDING_ALL(io.theme->padding1),
        //        .childGap = io.theme->childGap1,
        //        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
        //        .layoutDirection = CLAY_TOP_TO_BOTTOM
        //    },
        //    .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
        //    .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1),
        //    .floating = {.offset = {.x = 0, .y = static_cast<float>(io.theme->padding1)}, .attachPoints = {.element = CLAY_ATTACH_POINT_RIGHT_TOP, .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
        //}) {
        //    gui.obstructing_window(localId);
        //    text_label_centered(gui, "Grids");
        //    float entryHeight = 25.0f;
        //    if(main.world->gridMan.grids->empty())
        //        text_label_centered(gui, "No grids yet...");
        //    uint32_t toDelete = std::numeric_limits<uint32_t>::max();
        //    gui.scroll_bar_many_entries_area("grid menu entries", entryHeight, main.world->gridMan.grids->size(), true, [&](size_t i, bool isListHovered) {
        //        auto& grid = main.world->gridMan.grids->at(i)->obj;
        //        bool selectedEntry = gridMenu.selectedGrid == i;
        //        CLAY_AUTO_ID({
        //            .layout = {
        //                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(entryHeight)},
        //                .childGap = 2,
        //                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
        //                .layoutDirection = CLAY_LEFT_TO_RIGHT 
        //            },
        //            .backgroundColor = selectedEntry ? convert_vec4<Clay_Color>(io.theme->backColor1) : convert_vec4<Clay_Color>(io.theme->backColor2)
        //        }) {
        //            text_label(gui, grid->get_display_name());
        //            bool miniButtonClicked = false;
        //            CLAY_AUTO_ID({
        //                .layout = {
        //                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
        //                    .childGap = 1,
        //                    .childAlignment = {.x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER},
        //                    .layoutDirection = CLAY_LEFT_TO_RIGHT
        //                }
        //            }) {
        //                if(gui.svg_icon_button_transparent("visibility eye", grid->visible ? "data/icons/eyeopen.svg" : "data/icons/eyeclose.svg", false, entryHeight, false)) {
        //                    miniButtonClicked = true;
        //                    grid->visible = !grid->visible;
        //                    NetworkingObjects::generic_serialized_class_send_update_to_all<WorldGrid>(grid);
        //                }
        //                if(gui.svg_icon_button_transparent("edit pencil", "data/icons/pencil.svg", false, entryHeight, false)) {
        //                    miniButtonClicked = true;
        //                    main.world->drawProg.modify_grid(grid);
        //                    stop_displaying_grid_menu();
        //                }
        //                if(gui.svg_icon_button_transparent("delete trash", "data/icons/trash.svg", false, entryHeight, false)) {
        //                    miniButtonClicked = true;
        //                    toDelete = i;
        //                }
        //            }
        //            if(Clay_Hovered() && io.mouse.leftClick && isListHovered && !miniButtonClicked) {
        //                gridMenu.selectedGrid = i;
        //                if(io.mouse.leftClick >= 2) {
        //                    main.world->drawProg.modify_grid(grid);
        //                    stop_displaying_grid_menu();
        //                }
        //            }
        //        }
        //    });
        //    if(toDelete != std::numeric_limits<uint32_t>::max())
        //        main.world->gridMan.remove_grid(toDelete);
        //    gui.left_to_right_line_layout([&]() {
        //        bool addByEnter = false;
        //        gui.input_text("grid text input", &gridMenu.newName, true, [&](GUIStuff::SelectionHelper& s) {
        //            addByEnter = s.selected && io.key.enter;
        //        });
        //        if(gui.svg_icon_button("grid add button", "data/icons/plusbold.svg", false, GUIStuff::GUIManager::SMALL_BUTTON_SIZE) || (addByEnter && !gridMenu.newName.empty())) {
        //            main.world->gridMan.add_default_grid(gridMenu.newName);
        //            main.world->drawProg.modify_grid(main.world->gridMan.grids->at(main.world->gridMan.grids->size() - 1)->obj);
        //            stop_displaying_grid_menu();
        //        }
        //    });

        //    bool dropdownHover = false;
        //    if(io.mouse.leftClick && !Clay_Hovered() && !justOpened && !dropdownHover)
        //        stop_displaying_grid_menu();
        //}
        });
    }
}

void Toolbar::stop_displaying_grid_menu() {
    gridMenu.newName.clear();
    gridMenu.popupOpen = false;
    gridMenu.selectedGrid = std::numeric_limits<uint32_t>::max();
}

void Toolbar::bookmark_menu(bool justOpened) {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    gui.new_id("bookmark menu", [&] {
    //Clay_ElementId localId = CLAY_ID_LOCAL("INFINIPAINT BOOKMARK MENU");
    //CLAY(localId, {
    //    .layout = {
    //        .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0, 600) },
    //        .padding = CLAY_PADDING_ALL(io.theme->padding1),
    //        .childGap = io.theme->childGap1,
    //        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
    //        .layoutDirection = CLAY_TOP_TO_BOTTOM
    //    },
    //    .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
    //    .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1),
    //    .floating = {.offset = {.x = 0, .y = static_cast<float>(io.theme->padding1)}, .attachPoints = {.element = CLAY_ATTACH_POINT_RIGHT_TOP, .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
    //}) {
    //    gui.obstructing_window(localId);
    //    text_label_centered(gui, "Bookmarks");
    //    main.world->bMan.setup_list_gui("bookmark menu list");
    //    if(io.mouse.leftClick && !Clay_Hovered() && !justOpened) {
    //        bookmarkMenuPopupOpen = false;
    //        main.world->bMan.refresh_gui_data();
    //    }
    //}
    });
}

void Toolbar::layer_menu(bool justOpened) {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    gui.new_id("layer menu", [&] {
    //Clay_ElementId localId = CLAY_ID_LOCAL("INFINIPAINT LAYER MENU");
    //CLAY(localId, {
    //    .layout = {
    //        .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0, 600) },
    //        .padding = CLAY_PADDING_ALL(io.theme->padding1),
    //        .childGap = io.theme->childGap1,
    //        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
    //        .layoutDirection = CLAY_TOP_TO_BOTTOM
    //    },
    //    .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
    //    .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1),
    //    .floating = {.offset = {.x = 0, .y = static_cast<float>(io.theme->padding1)}, .attachPoints = {.element = CLAY_ATTACH_POINT_RIGHT_TOP, .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
    //}) {
    //    gui.obstructing_window(localId);
    //    text_label_centered(gui, "Layers");
    //    bool hoveringOverDropdown = false;
    //    main.world->drawProg.layerMan.listGUI.setup_list_gui("layer menu list", hoveringOverDropdown);
    //    if(io.mouse.leftClick && !Clay_Hovered() && !justOpened && !hoveringOverDropdown) {
    //        layerMenuPopupOpen = false;
    //        main.world->drawProg.layerMan.listGUI.refresh_gui_data();
    //    }
    //}
    });
}

std::unique_ptr<skia::textlayout::Paragraph> Toolbar::build_paragraph_from_chat_message(const ChatMessage& message, float alpha) {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    skia::textlayout::ParagraphStyle pStyle;
    pStyle.setTextAlign(skia::textlayout::TextAlign::kLeft);
    skia::textlayout::TextStyle tStyle;
    tStyle.setFontSize(io.fontSize);
    tStyle.setFontFamilies(main.fonts->get_default_font_families());
    tStyle.setFontStyle(SkFontStyle::Bold());
    tStyle.setForegroundColor(SkPaint{color_mul_alpha(message.type == ChatMessage::JOIN ? io.theme->warningColor : io.theme->frontColor1, alpha)});
    pStyle.setTextStyle(tStyle);

    skia::textlayout::ParagraphBuilderImpl a(pStyle, io.fonts->collection, SkUnicodes::ICU::Make());
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
    //constexpr float CHATBOX_WIDTH = 700;
    //if(main.world->netObjMan.is_connected()) {
    //    Clay_ElementId localId = CLAY_ID_LOCAL("INFINIPAINT CHAT BOX OPEN BUTTON");
    //    CLAY(localId, {
    //        .layout = {
    //            .layoutDirection = CLAY_LEFT_TO_RIGHT
    //        },
    //        .floating = {.offset = {static_cast<float>(io.theme->padding1), -static_cast<float>(io.theme->padding1)}, .attachPoints = {.element = CLAY_ATTACH_POINT_LEFT_BOTTOM, .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
    //    }) {
    //        gui.obstructing_window(localId);
    //        gui.push_id("chat box open button");
    //        if(gui.svg_icon_button("Chat Open Button", "data/icons/chat.svg", chatBoxState != CHATBOXSTATE_CLOSE)) {
    //            if(chatBoxState == CHATBOXSTATE_CLOSE)
    //                chatBoxState = CHATBOXSTATE_JUSTOPEN;
    //            else
    //                chatBoxState = CHATBOXSTATE_CLOSE;
    //        }
    //        gui.pop_id();
    //    }
    //}
    //Clay_ElementId localId = CLAY_ID_LOCAL("INFINIPAINT CHAT BOX OPEN");
    //CLAY(localId, {
    //    .layout = {
    //        .sizing = {.width = CLAY_SIZING_FIXED(CHATBOX_WIDTH), .height = CLAY_SIZING_FIT(0) },
    //        .childGap = 0,
    //        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM},
    //        .layoutDirection = CLAY_TOP_TO_BOTTOM
    //    },
    //    .floating = {.offset = {60 + static_cast<float>(io.theme->padding1), -static_cast<float>(io.theme->padding1)}, .attachPoints = {.element = CLAY_ATTACH_POINT_LEFT_BOTTOM, .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
    //}) {
    //    gui.obstructing_window(localId);
    //    gui.push_id("chat box");
    //    if(chatBoxState == CHATBOXSTATE_JUSTOPEN || chatBoxState == CHATBOXSTATE_OPEN) {
    //        gui.push_id("messages");
    //        CLAY_AUTO_ID({
    //            .layout = {
    //                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
    //                .padding = CLAY_PADDING_ALL(0),
    //                .childGap = 0,
    //                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM},
    //                .layoutDirection = CLAY_TOP_TO_BOTTOM
    //            },
    //            .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1)
    //        }) {
    //            int id = 0;
    //            for(auto& chatMessage : main.world->chatMessages | std::views::reverse) {
    //                gui.push_id(id);
    //                gui.text_paragraph("text", build_paragraph_from_chat_message(chatMessage, 1.0f), CHATBOX_WIDTH);
    //                gui.pop_id();
    //                id++;
    //            }
    //        }
    //        gui.pop_id();

    //        gui.left_to_right_line_layout([&]() {
    //            gui.input_text("message input", &chatMessageInput);
    //            if(gui.text_button("send button", "Send", false, true)) {
    //                if(!chatMessageInput.empty())
    //                    main.world->send_chat_message(chatMessageInput);
    //            }
    //        });
    //        gui.push_id("message input");
    //        GUIStuff::GUIManager::ElementContainer& a = gui.elements[gui.idStack];
    //        GUIStuff::TextBox<std::string>& t = *(GUIStuff::TextBox<std::string>*)a.elem.get();
    //        gui.pop_id();
    //        if(chatBoxState == CHATBOXSTATE_JUSTOPEN) {
    //            t.selection.selected = true;
    //            chatBoxState = CHATBOXSTATE_OPEN;
    //        }
    //        if(io.key.escape)
    //            t.selection.selected = false;
    //        if(io.key.enter) {
    //            main.world->send_chat_message(chatMessageInput);
    //            t.selection.selected = false;
    //        }
    //        if(!t.selection.selected) {
    //            chatMessageInput.clear();
    //            chatBoxState = CHATBOXSTATE_CLOSE;
    //        }
    //    }
    //    else {
    //        constexpr float DISPLAY_TIME = 8.0f;
    //        constexpr float FADE_START_TIME = 7.0f;
    //        gui.push_id("messages popup");
    //        int id = 0;
    //        for(auto& chatMessage : main.world->chatMessages | std::views::reverse) {
    //            chatMessage.time.update_time_since();
    //            if(chatMessage.time < DISPLAY_TIME) {
    //                float a = 1.0f - lerp_time<float>(chatMessage.time, DISPLAY_TIME, FADE_START_TIME);
    //                Clay_ElementId elemId = CLAY_IDI_LOCAL("CHAT MESSAGE ID", id);
    //                CLAY(elemId, {
    //                    .layout = {
    //                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
    //                        .padding = CLAY_PADDING_ALL(0),
    //                        .childGap = 0,
    //                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
    //                        .layoutDirection = CLAY_TOP_TO_BOTTOM
    //                    },
    //                    .backgroundColor = convert_vec4<Clay_Color>(color_mul_alpha(io.theme->backColor1, a)),
    //                }) {
    //                    gui.push_id(id);
    //                    gui.obstructing_window(elemId);
    //                    gui.text_paragraph("text", build_paragraph_from_chat_message(chatMessage, a), CHATBOX_WIDTH);
    //                    gui.pop_id();
    //                }
    //            }
    //            id++;
    //        }
    //        gui.pop_id();
    //    }
    //    gui.pop_id();
    //}
}

void Toolbar::global_log() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_FIT(0) },
            .childGap = io.theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .floating = {.offset = {0, 10}, .attachPoints = {.element = CLAY_ATTACH_POINT_RIGHT_TOP, .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
    //    constexpr float DISPLAY_TIME = 8.0f;
    //    constexpr float FADE_START_TIME = 7.0f;
    //    int i = 0;
    //    for(auto& logM : main.logMessages) {
    //        logM.time.update_time_since();
    //        if(logM.time < DISPLAY_TIME) {
    //            float a = 1.0f - lerp_time<float>(logM.time, DISPLAY_TIME, FADE_START_TIME);
    //            Clay_ElementId elemId = CLAY_IDI_LOCAL("GLOBAL LOG", i++);
    //            CLAY(elemId, {
    //                .layout = {
    //                    .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0) },
    //                    .padding = CLAY_PADDING_ALL(io.theme->padding1),
    //                    .childGap = 0,
    //                    .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
    //                    .layoutDirection = CLAY_TOP_TO_BOTTOM
    //                },
    //                .backgroundColor = convert_vec4<Clay_Color>(color_mul_alpha(io.theme->backColor1, a)),
    //                .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1)
    //            }) {
    //                gui.obstructing_window(elemId);
    //                SkColor4f c{0, 0, 0, 0};
    //                switch(logM.color) {
    //                    case LogMessage::COLOR_NORMAL:
    //                        c = io.theme->frontColor1;
    //                        break;
    //                    case LogMessage::COLOR_ERROR:
    //                        c = io.theme->errorColor;
    //                        break;
    //                }
    //                gui.text_label_color(logM.text, color_mul_alpha(c, a));
    //            }
    //        }
    //        else
    //            break;
    //    }
    }
}

void Toolbar::drawing_program_gui() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
            .childGap = io.theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
    }) {
        main.world->drawProg.toolbar_gui();
        isUpdatingColorLeft = isUpdatingColorRight = false;
        //if(colorLeft) {
        //    CLAY_AUTO_ID({
        //        .layout = {
        //            .padding = {.top = 40, .bottom = 40}
        //        }
        //    }) {
        //        auto localId = CLAY_ID_LOCAL("Drawing program gui color picker left");
        //        CLAY(localId, {
        //            .layout = {
        //                .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0)},
        //                .padding = CLAY_PADDING_ALL(io.theme->padding1),
        //                .childGap = io.theme->childGap1,
        //                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
        //                .layoutDirection = CLAY_TOP_TO_BOTTOM
        //            },
        //            .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
        //            .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1)
        //        }) {
        //            gui.obstructing_window(localId);
        //            isUpdatingColorLeft |= gui.color_picker_items("colorpickerleft", colorLeft, true, 300.0f - io.theme->padding1 * 2.0f);
        //            bool hoveringOnDropdown = false;
        //            isUpdatingColorLeft |= color_palette("colorpickerleftpalette", colorLeft, hoveringOnDropdown);
        //            if(!Clay_Hovered() && !justAssignedColorLeft && !hoveringOnDropdown && io.mouse.leftClick)
        //                colorLeft = nullptr;
        //        }
        //    }
        //}
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
            }
        }) {}
        //if(colorRight) {
        //    CLAY_AUTO_ID({
        //        .layout = {
        //            .padding = {.top = 40, .bottom = 40}
        //        }
        //    }) {
        //        auto localID = CLAY_ID_LOCAL("Drawing program gui color picker right");
        //        CLAY(localID, {
        //            .layout = {
        //                .sizing = {.width = CLAY_SIZING_FIT(300), .height = CLAY_SIZING_FIT(0)},
        //                .padding = CLAY_PADDING_ALL(io.theme->padding1),
        //                .childGap = io.theme->childGap1,
        //                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
        //                .layoutDirection = CLAY_TOP_TO_BOTTOM
        //            },
        //            .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
        //            .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1)
        //        }) {
        //            gui.obstructing_window(localID);
        //            isUpdatingColorRight |= gui.color_picker_items("colorpickerright", colorRight, true, 300.0f - io.theme->padding1 * 2.0f);
        //            bool hoveringOnDropdown = false;
        //            isUpdatingColorRight |= color_palette("colorpickerrightpalette", colorRight, hoveringOnDropdown);
        //            if(!Clay_Hovered() && !justAssignedColorRight && !hoveringOnDropdown && io.mouse.leftClick)
        //                colorRight = nullptr;
        //        }
        //    }
        //}
        main.world->drawProg.tool_options_gui();
    }
}

bool Toolbar::color_palette(const char* id, Vector4f* color, bool& hoveringOnDropdown) {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    bool isUpdating = false;
    gui.new_id(id, [&] {
    //auto& palette = paletteData.palettes[paletteData.selectedPalette].colors;
    //constexpr float COLOR_BUTTON_SIZE = GUIStuff::GUIManager::BIG_BUTTON_SIZE;

    //size_t nextID = 0;

    //gui.scroll_bar_area("color palette scroll area", false, [&](float, float, float &) {
    //    CLAY_AUTO_ID({
    //        .layout = {
    //            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
    //            .childGap = io.theme->childGap1,
    //            .layoutDirection = CLAY_TOP_TO_BOTTOM
    //        }
    //    }) {
    //        size_t i = 0;
    //        while(i < palette.size()) {
    //            CLAY_AUTO_ID({
    //                .layout = {
    //                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(COLOR_BUTTON_SIZE)},
    //                    .childGap = io.theme->childGap1,
    //                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
    //                    .layoutDirection = CLAY_LEFT_TO_RIGHT
    //                }
    //            }) {
    //                while(i < palette.size()) {
    //                    CLAY_AUTO_ID({
    //                        .layout = {
    //                            .sizing = {.width = CLAY_SIZING_FIXED(COLOR_BUTTON_SIZE), .height = CLAY_SIZING_FIXED(COLOR_BUTTON_SIZE)}
    //                        }
    //                    }) {
    //                        Vector4f newC = {palette[i].x(), palette[i].y(), palette[i].z(), 1.0f};
    //                        gui.push_id(nextID++);
    //                        if(gui.color_button("c", &newC, (paletteData.selectedColor == (int)i))) {
    //                            paletteData.selectedColor = (int)i;
    //                            // We want to keep the old color's alpha
    //                            color->x() = newC.x();
    //                            color->y() = newC.y();
    //                            color->z() = newC.z();
    //                            isUpdating = true;
    //                        }
    //                        gui.pop_id();
    //                        if(paletteData.selectedColor == (int)i && (newC.x() != color->x() || newC.y() != color->y() || newC.z() != color->z()))
    //                            paletteData.selectedColor = -1;
    //                    }
    //                    i++;
    //                    if(i % 6 == 0)
    //                        break;
    //                }
    //            }
    //        }
    //    }
    //});

    //if(paletteData.selectedPalette != 0) {
    //    CLAY_AUTO_ID({
    //        .layout = {
    //            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
    //            .padding = {.top = 3, .bottom = 3},
    //            .childGap = io.theme->childGap1,
    //            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
    //            .layoutDirection = CLAY_LEFT_TO_RIGHT
    //        }
    //    }) {
    //        if(gui.svg_icon_button("addcolor", "data/icons/plus.svg", false, COLOR_BUTTON_SIZE)) {
    //            palette.emplace_back(color->x(), color->y(), color->z());
    //            paletteData.selectedColor = palette.size() - 1;
    //        }
    //        if(gui.svg_icon_button("deletecolor", "data/icons/close.svg", false, COLOR_BUTTON_SIZE)) {
    //            if(paletteData.selectedColor >= 0 && paletteData.selectedColor < (int)palette.size()) {
    //                palette.erase(palette.begin() + paletteData.selectedColor);
    //                paletteData.selectedColor = -1;
    //            }
    //        }
    //    }
    //}
    //CLAY_AUTO_ID({
    //    .layout = {
    //        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
    //        .padding = {.top = 3, .bottom = 3},
    //        .childGap = io.theme->childGap1,
    //        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
    //        .layoutDirection = CLAY_LEFT_TO_RIGHT
    //    }
    //}) {
    //    std::vector<std::string> paletteNames;
    //    for(auto& p : paletteData.palettes)
    //        paletteNames.emplace_back(p.name);
    //    gui.dropdown_select("paletteselector", &paletteData.selectedPalette, paletteNames, 200.0f, [&]() {
    //        hoveringOnDropdown = Clay_Hovered();
    //    });
    //    if(gui.svg_icon_button("paletteadd", "data/icons/plus.svg", paletteData.addingPalette, 25.0f)) paletteData.addingPalette = !paletteData.addingPalette;
    //    if(paletteData.selectedPalette != 0) {
    //        if(gui.svg_icon_button("paletteremove", "data/icons/close.svg", false, 25.0f)) {
    //            paletteData.palettes.erase(paletteData.palettes.begin() + paletteData.selectedPalette);
    //            paletteData.selectedPalette = 0;
    //        }
    //    }
    //}
    //if(paletteData.addingPalette) {
    //    gui.input_text_field("paletteinputname", "Name", &paletteData.newPaletteStr);
    //    if(gui.text_button_wide("addpalettebutton", "Create") && !paletteData.newPaletteStr.empty()) {
    //        paletteData.palettes.emplace_back();
    //        paletteData.palettes.back().name = paletteData.newPaletteStr;
    //        paletteData.selectedPalette = paletteData.palettes.size() - 1;
    //        paletteData.addingPalette = false;
    //    }
    //}
    });

    return isUpdating;
}

void Toolbar::performance_metrics() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(io.theme->padding1),
            .childGap = io.theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM},
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
        .floating = {.offset = {-10, -10}, .attachPoints = {.element = CLAY_ATTACH_POINT_RIGHT_BOTTOM, .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM}, .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(io.theme->padding1),
                .childGap = io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = convert_vec4<Clay_Color>(color_mul_alpha(io.theme->backColor1, 0.7f)),
        }) {
            text_label(gui, "Undo queue");
            std::vector<std::string> undoList = main.world->undo.get_front_undo_queue_names(10);
            for(const std::string& u : undoList)
                text_label(gui, u);
        }
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(io.theme->padding1),
                .childGap = io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = convert_vec4<Clay_Color>(color_mul_alpha(io.theme->backColor1, 0.7f)),
        }) {
            std::stringstream a;
            a << "FPS: " << std::fixed << std::setprecision(0) << (1.0 / main.deltaTime);
            text_label(gui, a.str());
            text_label(gui, "Item Count: " + std::to_string(main.world->drawProg.layerMan.total_component_count()));
            std::stringstream b;
            b << "Coord: " << main.world->drawData.cam.c.pos.x().display_int_str(5) << ", " << main.world->drawData.cam.c.pos.y().display_int_str(5);
            text_label(gui, b.str());
            std::stringstream c;
            c << "Zoom: " << main.world->drawData.cam.c.inverseScale.display_int_str(5);
            text_label(gui, c.str());
            std::stringstream d;
            d << "Rotation: " << main.world->drawData.cam.c.rotation;
            text_label(gui, d.str());
        }
    }
}

void Toolbar::player_list() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(500), .height = CLAY_SIZING_FIT(0) },
            .padding = CLAY_PADDING_ALL(io.theme->padding1),
            .childGap = io.theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1),
        .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
    }) {
        gui.new_id("client list", [&] {
            text_label_centered(gui, "Player List");
            if(!main.world->clientStillConnecting) {
                left_to_right_line_layout(gui, [&]() {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)}
                        },
                        .backgroundColor = convert_vec4<Clay_Color>(SkColor4f{main.world->ownClientData->get_cursor_color().x(), main.world->ownClientData->get_cursor_color().y(), main.world->ownClientData->get_cursor_color().z(), 1.0f}),
                        .cornerRadius = CLAY_CORNER_RADIUS(3)
                    }) {}
                    text_label(gui, main.world->ownClientData->get_display_name());
                });
                size_t num = 0;
                for(auto& client : main.world->clients->get_data()) {
                    if(client != main.world->ownClientData) {
                        gui.new_id(num++, [&] {
                            left_to_right_line_layout(gui, [&]() {
                                CLAY_AUTO_ID({
                                    .layout = {
                                        .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)}
                                    },
                                    .backgroundColor = convert_vec4<Clay_Color>(SkColor4f{client->get_cursor_color().x(), client->get_cursor_color().y(), client->get_cursor_color().z(), 1.0f}),
                                    .cornerRadius = CLAY_CORNER_RADIUS(3)
                                }) {}
                                text_label(gui, client->get_display_name());
                                CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
                                text_button(gui, "teleport button", "Jump To", { .onClick = [&] {
                                    main.world->drawData.cam.smooth_move_to(*main.world, client->get_cam_coords(), client->get_window_size());
                                }});
                            });
                        });
                    }
                }
            }
            text_button(gui, "close list", "Done", { .wide = true, .onClick = [&] {
                playerMenuOpen = false;
            }});
        });
    }
}

void Toolbar::open_world_file(bool isClient, const std::string& netSource, const std::string& serverLocalID2) {
#ifdef __EMSCRIPTEN__
    static struct UploadData {
        bool iC;
        std::string nS;
        std::string sLID;
        MainProgram* main;
    } uploadData;
    uploadData.iC = isClient;
    uploadData.nS = netSource;
    uploadData.sLID = serverLocalID2;
    uploadData.main = &main;
    emscripten_browser_file::upload("." + World::FILE_EXTENSION, [](std::string const& fileName, std::string const& mimeType, std::string_view buffer, void* callbackData) {
        if(!buffer.empty()) {
            UploadData* uD = (UploadData*)callbackData;
            uD->main->new_tab({
                .isClient = uD->iC,
                .filePathSource = std::filesystem::path(fileName),
                .netSource = uD->nS,
                .serverLocalID = uD->sLID,
                .fileDataBuffer = buffer
            }, true);
        }
    }, &uploadData);
#else
    open_file_selector("Open", {{"InfiniPaint Canvas", World::FILE_EXTENSION}, {"Any File", "*"}}, [&, isClient = isClient, netSource = netSource, serverLocalID2 = serverLocalID2](const std::filesystem::path& p, const auto& e) {
        main.new_tab({
            .isClient = isClient,
            .filePathSource = p,
            .netSource = netSource,
            .serverLocalID = serverLocalID2
        });
    });
#endif
}

void Toolbar::center_obstructing_window_gui(const char* id, Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& innerContent) {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    gui.element<LayoutElement>(id, [&] (const Clay_ElementId& id) {
        CLAY(id, {
            .layout = {
                .sizing = {.width = x, .height = y },
                .padding = CLAY_PADDING_ALL(io.theme->padding1),
                .childGap = io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
            .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1),
            .floating = {.attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
        }) {
            innerContent();
        }
    });
}

void Toolbar::text_button_wide(const char* id, const char* str, const std::function<void()>& onClick) {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    text_button(gui, id, str, {
        .wide = true,
        .onClick = onClick
    });
}

void Toolbar::options_menu() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    switch(optionsMenuType) {
        case HOST_MENU: {
            center_obstructing_window_gui("host menu", CLAY_SIZING_FIT(650), CLAY_SIZING_FIT(0), [&] {
                input_text_field(gui, "lobby", "Lobby", &serverToConnectTo);
                left_to_right_line_layout(gui, [&]() {
                    text_button_wide("copy lobby address", "Copy Lobby Address", [&] {
                        main.input.set_clipboard_str(serverToConnectTo);
                    });
                    text_button_wide("host file", "Host", [&] {
                        main.world->start_hosting(serverToConnectTo, serverLocalID);
                        optionsMenuOpen = false;
                    });
                    text_button_wide("cancel", "Cancel", [&] {
                        optionsMenuOpen = false;
                    });
                });
            });
            break;
        }
        case CONNECT_MENU: {
            center_obstructing_window_gui("connect menu", CLAY_SIZING_FIT(650), CLAY_SIZING_FIT(0), [&] {
                input_text_field(gui, "lobby", "Lobby", &serverToConnectTo);
                left_to_right_line_layout(gui, [&]() {
                    text_button_wide("connect", "Connect", [&] {
                        if(serverToConnectTo.length() != (NetLibrary::LOCALID_LEN + NetLibrary::GLOBALID_LEN))
                            Logger::get().log("USERINFO", "Connect issue: Incorrect address length");
                        else if(serverToConnectTo.substr(0, NetLibrary::GLOBALID_LEN) == NetLibrary::get_global_id())
                            Logger::get().log("USERINFO", "Connect issue: Can't connect to your own address");
                        else {
                            main.new_tab({
                                .isClient = true,
                                .netSource = serverToConnectTo
                            }, true);
                            optionsMenuOpen = false;
                        }
                    });
                    text_button_wide("cancel", "Cancel", [&] {
                        optionsMenuOpen = false;
                    });
                });
            });
            break;
        }
        case GENERAL_SETTINGS_MENU: {
            center_obstructing_window_gui("gsettings", CLAY_SIZING_FIXED(600), CLAY_SIZING_FIXED(500), [&] {
                general_settings_inner_gui();
            });
            break;
        }
        case LOBBY_INFO_MENU: {
            center_obstructing_window_gui("lobby info menu", CLAY_SIZING_FIT(650), CLAY_SIZING_FIT(0), [&] {
                input_text_field(gui, "lobby", "Lobby", &main.world->netSource);
                left_to_right_line_layout(gui, [&]() {
                    text_button_wide("copy lobby address", "Copy Lobby Address", [&] {
                        main.input.set_clipboard_str(main.world->netSource);
                    });
                    text_button_wide("done", "Done", [&] {
                        optionsMenuOpen = false;
                    });
                });
            });
            break;
        }
        case CANVAS_SETTINGS_MENU: {
            center_obstructing_window_gui("canvas settings menu", CLAY_SIZING_FIT(500), CLAY_SIZING_FIT(0), [&] {
                auto newColorToSet = std::make_shared<SkColor4f>(main.world->canvasTheme.get_back_color());
                color_picker_button_field(gui, "canvasColor", "Canvas Color", newColorToSet.get(), {
                    .hasAlpha = false,
                    .onEdit = [&, newColorToSet] {
                        main.world->canvasTheme.set_back_color(convert_vec3<Vector3f>(*newColorToSet));
                    }
                });
                text_button_wide("done", "Done", [&] {
                    optionsMenuOpen = false;
                });
            });
            break;
        }
        case SET_DOWNLOAD_NAME: {
            center_obstructing_window_gui("set download name menu", CLAY_SIZING_FIT(500), CLAY_SIZING_FIT(0), [&] {
                input_text_field(gui, "file name", "File Name", &downloadNameSet);
                left_to_right_line_layout(gui, [&]() {
                    text_button_wide("download save button", "Save", [&] {
                        main.world->save_to_file(downloadNameSet);
                        optionsMenuOpen = false;
                    });
                    text_button_wide("cancel", "Cancel", [&] {
                        optionsMenuOpen = false;
                    });
                });
            });
            break;
        }
        case ABOUT_MENU: {
            center_obstructing_window_gui("about menu", CLAY_SIZING_FIXED(650), CLAY_SIZING_FIXED(500), [&] {
                about_menu_inner_gui();
            });
            break;
        }
    }
}

void Toolbar::general_settings_inner_gui() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
            .padding = CLAY_PADDING_ALL(io.theme->padding1),
            .childGap = io.theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        }
    }) {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIT(150), .height = CLAY_SIZING_GROW(0) },
                .padding = CLAY_PADDING_ALL(io.theme->padding1),
                .childGap = 2,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }
        }) {
            auto category_button = [&](const char* id, const char* str, GeneralSettingsOptions opt) {
                text_button(gui, id, str, {
                    .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                    .isSelected = generalSettingsOptions == opt,
                    .wide = true,
                    .centered = false,
                    .onClick = [&, opt] {
                        main.g.load_theme(main.configPath, main.conf.themeCurrentlyLoaded);
                        themeData.selectedThemeIndex = std::nullopt;
                        generalSettingsOptions = opt;
                    }
                });
            };
            category_button("Generalbutton", "General", GSETTINGS_GENERAL);
            category_button("Tabletbutton", "Tablet", GSETTINGS_TABLET);
            category_button("Themebutton", "Theme", GSETTINGS_THEME);
            category_button("Keybindsbutton", "Keybinds", GSETTINGS_KEYBINDS);
            category_button("Debugbutton", "Debug", GSETTINGS_DEBUG);
        }
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }
        }) {
            auto general_scroll_area = [&](const char* id, const std::function<void()>& innerContent) {
                gui.clipping_element<ScrollArea>("general settings scroll area", ScrollArea::Options{
                    .scrollVertical = true,
                    .clipHorizontal = false,
                    .clipVertical = true,
                    .showScrollbarY = true,
                    .innerContent = [&, innerContent](const ScrollArea::InnerContentParameters&) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                                .padding = CLAY_PADDING_ALL(io.theme->padding1),
                                .childGap = io.theme->childGap1,
                                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM
                            }
                        }) {
                            innerContent();
                        }
                    }
                });
            };
            switch(generalSettingsOptions) {
                case GSETTINGS_GENERAL: {
                    general_scroll_area("general settings", [&] {
                        input_text_field(gui, "display name input", "Display name", &main.conf.displayName);
                        main.update_display_names();
                        color_picker_button_field(gui, "defaultCanvasBackgroundColor", "Default canvas background color", &main.conf.defaultCanvasBackgroundColor, { .hasAlpha = false });
                        #ifndef __EMSCRIPTEN__
                            checkbox_boolean_field(gui, "native file pick", "Use native file picker", &main.conf.useNativeFilePicker);
                            checkbox_boolean_field(gui, "update notifications enable", "Check for updates on startup", &main.conf.checkForUpdates);
                        #endif
                        slider_scalar_field(gui, "drag zoom slider", "Drag zoom speed", &main.conf.dragZoomSpeed, 0.0, 1.0, {.decimalPrecision = 3});
                        slider_scalar_field(gui, "scroll zoom slider", "Scroll zoom speed", &main.conf.scrollZoomSpeed, 0.0, 1.0, {.decimalPrecision = 3});
                        checkbox_boolean_field(gui, "flip zoom tool direction", "Flip zoom tool direction", &main.conf.flipZoomToolDirection);
                        checkbox_boolean_field(gui, "make all tools share same size", "Make all tools share size", &main.toolConfig.globalConf.useGlobalRelativeWidth);
                        #ifndef __EMSCRIPTEN__
                            checkbox_boolean_field(gui, "disable graphics driver workarounds", "Disable graphics driver workarounds (enabling or disabling this might fix some graphical glitches, requires restart)", &main.conf.disableGraphicsDriverWorkarounds);
                        #endif
                        input_scalar_field(gui, "jump transition time", "Jump transition time", &main.conf.jumpTransitionTime, 0.01f, 1000.0f, {.decimalPrecision = 2});
                        input_scalar_field(gui, "Max GUI Scale", "Max GUI Scale", &main.conf.guiScale, 0.5f, 5.0f, {.decimalPrecision = 1});
                        text_label(gui, "Anti-aliasing:");
                        radio_button_selector(gui, "Antialiasing selector", &main.conf.antialiasing, {
                            {"None", GlobalConfig::AntiAliasing::NONE},
                            {"Skia", GlobalConfig::AntiAliasing::SKIA},
                            {"Dynamic MSAA", GlobalConfig::AntiAliasing::DYNAMIC_MSAA}
                        }, [&] {
                            main.refresh_draw_surfaces();
                        });
                        text_label(gui, "VSync:");
                        radio_button_selector(gui, "VSync selector", &main.conf.vsyncValue, {
                            {"On", 1},
                            {"Off", 0},
                            {"Adaptive", -1}
                        }, [&] {
                            main.set_vsync_value(main.conf.vsyncValue);
                        });

                        #ifndef __EMSCRIPTEN__
                        checkbox_boolean_field(gui, "apply display scale", "Apply display scale", &main.conf.applyDisplayScale);
                        #endif
                    });
                    break;
                }
                case GSETTINGS_TABLET: {
                    general_scroll_area("tablet settings", [&] {
                        checkbox_boolean_field(gui, "pen pressure width", "Pen pressure affects brush size", &main.conf.tabletOptions.pressureAffectsBrushWidth);
                        slider_scalar_field(gui, "smoothing time", "Smoothing sampling time", &main.conf.tabletOptions.smoothingSamplingTime, 0.001f, 1.0f, {.decimalPrecision = 3});
                        input_scalar_field<uint8_t>(gui, "middle click", "Middle click pen button", &main.conf.tabletOptions.middleClickButton, 1, 255);
                        input_scalar_field<uint8_t>(gui, "right click", "Right click pen button", &main.conf.tabletOptions.rightClickButton, 1, 255);
                        slider_scalar_field(gui, "tablet brush minimum size", "Brush relative minimum size", &main.conf.tabletOptions.brushMinimumSize, 0.0f, 1.0f, {.decimalPrecision = 3});
                        checkbox_boolean_field(gui, "tablet zoom with button method", "Zoom when pen touching tablet and pen button assigned to middle click is held", &main.conf.tabletOptions.zoomWhilePenDownAndButtonHeld);
                        #ifdef _WIN32
                            checkbox_boolean_field(gui, "mouse ignore when pen proximity", "Ignore mouse movement when pen in proximity", &tabletOptions.ignoreMouseMovementWhenPenInProximity);
                        #endif
                    });
                    break;
                }
                case GSETTINGS_THEME: {
                    general_scroll_area("theme", [&] {
                        if(!themeData.selectedThemeIndex)
                            reload_theme_list();

                        CLAY_AUTO_ID({.layout = { 
                              .childGap = io.theme->childGap1,
                              .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                              .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        }
                        }) {
                            text_label(gui, "Theme: ");
                            gui.element<DropDown<size_t>>("dropdownSelectThemes", &themeData.selectedThemeIndex.value(), themeData.themeDirList, DropdownOptions{
                                .onClick = [&]() {
                                    main.conf.themeCurrentlyLoaded = themeData.themeDirList[themeData.selectedThemeIndex.value()];
                                    reload_theme_list();
                                }
                            });
                        }
                        left_to_right_line_layout(gui, [&]() {
                            if(themeData.selectedThemeIndex != 0) {
                                text_button_wide("savethemebutton", "Save", [&] {
                                    main.conf.themeCurrentlyLoaded = themeData.themeDirList[themeData.selectedThemeIndex.value()];
                                    main.g.save_theme(main.configPath, main.conf.themeCurrentlyLoaded);
                                    reload_theme_list();
                                });
                            }
                            text_button_wide("saveasthemebutton", "Save As", [&]() {
                                themeData.openedSaveAsMenu = !themeData.openedSaveAsMenu;
                            });
                            text_button_wide("reloadthemebutton", "Reload", [&] {
                                main.conf.themeCurrentlyLoaded = themeData.themeDirList[themeData.selectedThemeIndex.value()];
                                reload_theme_list();
                            });
                            if(themeData.selectedThemeIndex != 0) {
                                text_button_wide("deletethemebutton", "Delete", [&] {
                                    try { std::filesystem::remove(main.configPath / "themes" / (themeData.themeDirList[themeData.selectedThemeIndex.value()] + ".json")); } catch(...) { }
                                    main.conf.themeCurrentlyLoaded = "Default";
                                    reload_theme_list();
                                });
                            }
                        });
                        if(themeData.openedSaveAsMenu) {
                            input_text_field(gui, "Theme name:", "Theme name: ", &main.conf.themeCurrentlyLoaded);
                            left_to_right_line_layout(gui, [&]() {
                                text_button_wide("saveasdone", "Done", [&] {
                                    main.g.save_theme(main.configPath, main.conf.themeCurrentlyLoaded);
                                    reload_theme_list();
                                });
                                text_button_wide("saveascancel", "Cancel", [&] {
                                    themeData.openedSaveAsMenu = false;
                                });
                            });
                        }
                        text_label(gui, "Edit theme:");
                        text_label_light(gui, "Note: Changes only remain if theme is saved");
                        auto theme_color_field = [&](const char* id, const char* name, SkColor4f* c) {
                            color_picker_button_field<SkColor4f>(gui, id, name, c, {});
                        };
                        theme_color_field("fillColor2", "Fill Color 2", &io.theme->fillColor2);
                        theme_color_field("backColor1", "Back Color 1", &io.theme->backColor1);
                        theme_color_field("backColor2", "Back Color 2", &io.theme->backColor2);
                        theme_color_field("frontColor1", "Front Color 1", &io.theme->frontColor1);
                        theme_color_field("frontColor2", "Front Color 2", &io.theme->frontColor2);
                        theme_color_field("warningColor", "Warning Color", &io.theme->warningColor);
                        theme_color_field("errorColor", "Error Color", &io.theme->errorColor);
                        //gui.slider_scalar_field("hoverExpandTime", "Hover Expand Time", &io.theme->hoverExpandTime, 0.001f, 1.0f);
                        input_scalar_field<uint16_t>(gui, "childGap1", "Gap between child elements", &io.theme->childGap1, 0, 30);
                        input_scalar_field<uint16_t>(gui, "padding1", "Window padding", &io.theme->padding1, 0, 30);
                        slider_scalar_field<float>(gui, "windowCorners1", "Window corner radius", &io.theme->windowCorners1, 0, 30);
                    });
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

                    general_scroll_area("keybind entries", [&] {
                        for(unsigned i = 0; i < InputManager::KEY_ASSIGNABLE_COUNT; i++) {
                            gui.new_id(i, [&] {
                                CLAY_AUTO_ID({
                                    .layout = {
                                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                        .padding = CLAY_PADDING_ALL(0),
                                        .childGap = io.theme->childGap1,
                                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT 
                                    }
                                }) {
                                    text_label(gui, std::string(nlohmann::json(static_cast<InputManager::KeyCodeEnum>(i))));
                                    auto f = std::find_if(main.input.keyAssignments.begin(), main.input.keyAssignments.end(), [&](auto& p) {
                                        return p.second == i;
                                    });
                                    std::string assignedKeystrokeStr = f != main.input.keyAssignments.end() ? main.input.key_assignment_to_str(f->first) : "";
                                    text_button(gui, "keybind button", assignedKeystrokeStr, {
                                        .isSelected = keybindWaiting.has_value() && keybindWaiting.value() == i,
                                        .wide = true,
                                        .onClick = [&] {
                                            keybindWaiting = i;
                                        }
                                    });
                                }
                            });
                        }
                    });
                    break;
                }
                case GSETTINGS_DEBUG: {
                    general_scroll_area("debug settings menu", [&] {
                        checkbox_boolean_field(gui, "show performance metrics", "Show metrics", &showPerformance);
                        input_scalars_field(gui, "jump transition easing", "Jump easing", &main.conf.jumpTransitionEasing, 4, -10.0f, 10.0f, { .decimalPrecision = 2 });
                        #ifndef __EMSCRIPTEN__
                            input_scalar_field(gui, "fps cap slider", "FPS cap", &main.conf.fpsLimit, 3.0f, 10000.0f);
                        #endif
                        input_scalar_field<int>(gui, "image load max threads", "Maximum image loading threads", &ImageResourceDisplay::IMAGE_LOAD_THREAD_COUNT_MAX, 1, 10000);
                        text_label_light(gui, "Cache related settings");
                        input_scalar_field<size_t>(gui, "cache node resolution", "Cache node resolution", &DrawingProgramCache::CACHE_NODE_RESOLUTION, 256, 8192);
                        input_scalar_field<size_t>(gui, "max cache nodes", "Maximum cached nodes", &DrawingProgramCache::MAXIMUM_DRAW_CACHE_SURFACES, 2, 10000);
                        size_t cacheVRAMConsumptionInMB =  ( DrawingProgramCache::MAXIMUM_DRAW_CACHE_SURFACES // Number of surfaces
                                                           * DrawingProgramCache::CACHE_NODE_RESOLUTION * DrawingProgramCache::CACHE_NODE_RESOLUTION // Number of pixels per cache surface
                                                           * 4) // 4 Channels per pixel (RGBA)
                                                           / (1024 * 1024); // Bytes -> Megabytes conversion
                        text_label_light(gui, "Cache max VRAM consumption (MB): " + std::to_string(cacheVRAMConsumptionInMB));
                        input_scalar_field<size_t>(gui, "max components in node", "Maximum components in single node", &DrawingProgramCache::MAXIMUM_COMPONENTS_IN_SINGLE_NODE, 2, 10000);
                        input_scalar_field<size_t>(gui, "components to force cache rebuild", "Number of components to force cache rebuild", &DrawingProgramCache::MINIMUM_COMPONENTS_TO_START_REBUILD, 1, 1000000);
                        input_scalar_field<size_t>(gui, "maximum frame time to force cache rebuild", "Maximum frame time to force cache rebuild (ms)", &DrawingProgramCache::MILLISECOND_FRAME_TIME_TO_FORCE_CACHE_REFRESH, 1, 1000000);
                        input_scalar_field<size_t>(gui, "minimum time to force cache rebuild", "Minimum time to check cache rebuild (ms)", &DrawingProgramCache::MILLISECOND_MINIMUM_TIME_TO_CHECK_FORCE_REFRESH, 1, 1000000);
                    });
                    break;
                }
            }
            text_button_wide("done menu", "Done", [&] {
                main.save_config();
                main.g.load_theme(main.configPath, main.conf.themeCurrentlyLoaded);
                themeData.selectedThemeIndex = std::nullopt;
                optionsMenuOpen = false;
            });
        }
    }
}

void Toolbar::about_menu_inner_gui() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    left_to_right_line_layout(gui, [&]() {
        gui.new_id("Menu Selector", [&] {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(200), .height = CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(io.theme->padding1),
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                }
            }) {
                gui.clipping_element<ScrollArea>("About Menu Selector Scroll Area", ScrollArea::Options{
                    .scrollVertical = true,
                    .clipHorizontal = false,
                    .clipVertical = true,
                    .showScrollbarY = true,
                    .innerContent = [&](const ScrollArea::InnerContentParameters&) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)}
                            }
                        }) {
                            text_button(gui, "infinipaintnoticebutton", "InfiniPaint", {
                                .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                .isSelected = selectedLicense == -1,
                                .wide = true,
                                .centered = false,
                                .onClick = [&] { selectedLicense = -1; }
                            });
                        }
                        text_label_light_centered(gui, "Third Party Components");
                        for(int i = 0; i < static_cast<int>(thirdPartyLicenses.size()); i++) {
                            CLAY_AUTO_ID({
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                    .padding = CLAY_PADDING_ALL(1)
                                }
                            }) {
                                gui.new_id(i, [&] {
                                    text_button(gui, "noticebutton", thirdPartyLicenses[i].first, {
                                        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                        .isSelected = selectedLicense == i,
                                        .wide = true,
                                        .centered = false,
                                        .onClick = [&, i] { selectedLicense = i; }
                                    });
                                });
                            }
                        }
                    }
                });
            }
        });

        gui.clipping_element<ScrollArea>("About Menu Text Scroll Area", ScrollArea::Options{
            .scrollVertical = true,
            .clipHorizontal = false,
            .clipVertical = true,
            .showScrollbarY = true,
            .innerContent = [&](const ScrollArea::InnerContentParameters&) {
                text_label_size(gui, (selectedLicense == -1) ? ownLicenseText : thirdPartyLicenses[selectedLicense].second, 0.8f);
            }
        });
    });
    text_button_wide("done", "Done", [&] {
        optionsMenuOpen = false;
    });
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
                if(name == main.conf.themeCurrentlyLoaded)
                    themeData.selectedThemeIndex = themeData.themeDirList.size() - 1;
            }
        }
    }
    if(!main.g.load_theme(main.configPath, main.conf.themeCurrentlyLoaded))
        themeData.selectedThemeIndex = 0;

    themeData.openedSaveAsMenu = false;
}

void Toolbar::file_picker_gui_refresh_entries() {
    auto& gui = main.g.gui;

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

    gui.set_to_layout();
}

void Toolbar::file_picker_gui_done() {
    if(!filePicker.fileName.empty()) {
        std::filesystem::path pathToRet = filePicker.currentSearchPath / filePicker.fileName;
        if(filePicker.isSaving)
            pathToRet = force_extension_on_path(pathToRet, filePicker.extensionFiltersComplete[filePicker.extensionSelected].extensions);
        filePicker.postSelectionFunc(pathToRet, filePicker.extensionFiltersComplete[filePicker.extensionSelected]);
    }
    filePicker.isOpen = false;
}

void Toolbar::file_picker_gui() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    center_obstructing_window_gui("file picker gui window", CLAY_SIZING_FIXED(700), CLAY_SIZING_FIXED(500), [&] {
        text_label_centered(gui, filePicker.filePickerWindowName);
        left_to_right_line_layout(gui, [&]() {
            svg_icon_button(gui, "file picker back button", "data/icons/backarrow.svg", {
                .onClick = [&] {
                    filePicker.currentSearchPath = filePicker.currentSearchPath.parent_path();
                    file_picker_gui_refresh_entries();
                }
            });
            input_path_field(gui, "file picker path", "Path", &filePicker.currentSearchPath, {
                .onEdit = [&] {
                    file_picker_gui_refresh_entries();
                }
            });
        });
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor2)
        }) {
            constexpr float entryHeight = 25.0f;
            scroll_area_many_entries(gui, "file picker entries", {
                .entryHeight = entryHeight,
                .entryCount = filePicker.entries.size(),
                .clipHorizontal = false,
                .elementContent = [&] (size_t i) {
                    const std::filesystem::path& entry = filePicker.entries[i];
                    bool selectedEntry = filePicker.currentSelectedPath == entry;
                    gui.element<LayoutElement>("elem", [&] (const Clay_ElementId& lId) {
                        CLAY(lId, {
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(entryHeight)},
                                .childGap = 1,
                                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT 
                            },
                            .backgroundColor = selectedEntry ? convert_vec4<Clay_Color>(io.theme->backColor1) : convert_vec4<Clay_Color>(io.theme->backColor2)
                        }) {
                            CLAY_AUTO_ID({
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)}
                                },
                            }) {
                                if(std::filesystem::is_directory(entry))
                                    gui.element<SVGIcon>("folder icon", "data/icons/folder.svg", selectedEntry);
                                else
                                    gui.element<SVGIcon>("file icon", "data/icons/file.svg", selectedEntry);
                            }
                            text_label(gui, entry.filename().string());
                        }
                    }, LayoutElement::Callbacks {
                        .mouseButton = [&, selectedEntry, entry](const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) {
                            if(mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down) {
                                gui.set_post_callback_func([&, button, selectedEntry, entry] {
                                    if(selectedEntry && button.clicks >= 2) {
                                        if(std::filesystem::is_directory(entry)) {
                                            filePicker.currentSearchPath = entry;
                                            file_picker_gui_refresh_entries();
                                        }
                                        else if(std::filesystem::is_regular_file(entry))
                                            file_picker_gui_done();
                                    }
                                    else {
                                        filePicker.currentSelectedPath = entry;
                                        if(std::filesystem::is_regular_file(entry))
                                            filePicker.fileName = entry.filename().string();
                                    }
                                });
                                gui.set_to_layout();
                            }
                            return mouseHovering;
                        }
                    });
                }
            });
        }
        left_to_right_line_layout(gui, [&]() {
            input_text(gui, "filepicker filename", &filePicker.fileName);
            gui.element<DropDown<size_t>>("filepicker select type", &filePicker.extensionSelected, filePicker.extensionFilters, DropdownOptions{
                .onClick = [&] { file_picker_gui_refresh_entries(); }
            });
        });
        left_to_right_line_layout(gui, [&]() {
            text_button_wide("filepicker done", "Done", [&] { file_picker_gui_done(); });
            text_button_wide("filepicker cancel", "Cancel", [&] { filePicker.isOpen = false; });
        });
    });
}
