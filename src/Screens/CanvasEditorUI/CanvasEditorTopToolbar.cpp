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

#include "CanvasEditorTopToolbar.hpp"
#include "../../MainProgram.hpp"
#include "../../GUIStuff/Elements/LayoutElement.hpp"
#include "../../GUIStuff/Elements/SVGIcon.hpp"
#include "../../GUIStuff/Elements/MovableTabList.hpp"
#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/TextBoxHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "../../GUIStuff/Elements/ScrollArea.hpp"
#include "Helpers/ConvertVec.hpp"

using namespace GUIStuff;
using namespace ElementHelpers;

CanvasEditorTopToolbar::CanvasEditorTopToolbar(CanvasEditorScreen& initScreen):
    s(initScreen)
{}

void CanvasEditorTopToolbar::safety_layout_checks() {
    auto& main = s.main;

    if(!main.world->netServer) {
        if(settingsPopup == SettingsPopup::LOBBY_INFO)
            settingsPopup = SettingsPopup::NONE;
    }
    if(!main.world->netClient && !main.world->netServer) {
        if(settingsPopup == SettingsPopup::PLAYER_LIST)
            settingsPopup = SettingsPopup::NONE;
    }
}

void CanvasEditorTopToolbar::layout_top_toolbar() {
    auto& gui = s.main.g.gui;
    auto& io = gui.io;

    safety_layout_checks();

    window_fill_side_bar(gui, WindowFillSideBarConfig{
        .dir = WindowFillSideBarConfig::Direction::TOP
    }, [&] {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(static_cast<uint16_t>(s.mobileUI ? 0 : io.theme->padding1))
            },
        }) {
            gui.element<LayoutElement>("top menu bar", [&] (LayoutElement*, const Clay_ElementId& lId) {
                CLAY(lId, {
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                        .childGap = static_cast<uint16_t>(io.theme->childGap1 / 2),
                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                    },
                    .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
                    .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1)
                }) {
                    if(s.mobileUI) {
                        svg_icon_button(gui, "back exit button", "data/icons/RemixIcon/arrow-left-s-line.svg", {
                            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                            .onClick = [&] {
                                s.save_to_file();
                                s.main.set_tab_to_close(s.main.world.get());
                            }
                        });
                    }
                    else {
                        Element* b = svg_icon_button(gui, "main menu button", "data/icons/menu.svg", {
                            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                            .isSelected = desktopMainMenuPopupOpen || settingsPopup == SettingsPopup::MAIN_SETTINGS,
                            .onClick = [&] {
                                if(settingsPopup == SettingsPopup::MAIN_SETTINGS)
                                    settingsPopup = SettingsPopup::NONE;
                                desktopMainMenuPopupOpen = !desktopMainMenuPopupOpen;
                            }
                        });
                        if(desktopMainMenuPopupOpen)
                            desktop_main_menu_popup(b);
                    }
                    global_log();
                    if(!(s.main.world->netClient && s.main.world->clientStillConnecting)) {
                        layout_settings_popup();
                        remaining_area();
                    }
                }
            });
        }
    });
}

void CanvasEditorTopToolbar::desktop_main_menu_popup(GUIStuff::Element* b) {
    auto& main = s.main;
    auto& gui = main.g.gui;

    std::vector<IconMenuButton> listOfTopToolbarButtons;
    listOfTopToolbarButtons.emplace_back(IconMenuButton{
        .name = "New File",
        .svgPath = "data/icons/RemixIcon/file-add-line.svg",
        .onClick = [&] {
            CustomEvents::emit_event<CustomEvents::OpenInfiniPaintFileEvent>({
                .isClient = false
            });
        }
    });
    listOfTopToolbarButtons.emplace_back(IconMenuButton{
        .name = "Open",
        .svgPath = "data/icons/RemixIcon/file-3-line.svg",
        .onClick = [&] {
            s.open_world_file(false, "", "");
        }
    });
    if(!s.main.world->clientStillConnecting) {
        listOfTopToolbarButtons.emplace_back(IconMenuButton{
            .name = "Save",
            .svgPath = "data/icons/RemixIcon/save-line.svg",
            .onClick = [&] {
                s.desktop_save_file();
            }
        });
        listOfTopToolbarButtons.emplace_back(IconMenuButton{
            .name = "Save As",
            .svgPath = "data/icons/RemixIcon/save-line.svg",
            .onClick = [&] {
                s.desktop_save_as_file();
            }
        });
    }
    listOfTopToolbarButtons.emplace_back(IconMenuButton{
        .name = "Settings",
        .svgPath = "data/icons/RemixIcon/settings-3-line.svg",
        .onClick = [&] {
            settingsPopup = SettingsPopup::MAIN_SETTINGS;
        }
    });
    listOfTopToolbarButtons.emplace_back(IconMenuButton{
        .name = "Website",
        .svgPath = "data/icons/RemixIcon/global-line.svg",
        .onClick = [&] {
            SDL_OpenURL("https://infinipaint.com/");
        }
    });
    listOfTopToolbarButtons.emplace_back(IconMenuButton{
        .name = "Donate",
        .svgPath = "data/icons/RemixIcon/hand-coin-line.svg",
        .onClick = [&] {
            SDL_OpenURL("https://infinipaint.com/donate.html");
        }
    });
    #ifndef __EMSCRIPTEN__
        listOfTopToolbarButtons.emplace_back(IconMenuButton{
            .name = "Quit",
            .svgPath = "data/icons/RemixIcon/logout-box-line.svg",
            .onClick = [&] {
                if(main.app_close_requested())
                    main.setToQuit = true;
            }
        });
    #endif

    icon_menu_dropdown(b, "top toolbar main menu popup", gui.get_z_index() + 30, listOfTopToolbarButtons, [&] { desktopMainMenuPopupOpen = false; });
}

void CanvasEditorTopToolbar::layout_settings_popup() {
    auto& main = s.main;
    auto& world = *s.main.world;
    auto& gui = main.g.gui;

    switch(settingsPopup) {
        case SettingsPopup::NONE:
        case SettingsPopup::HIDDEN_BUTTONS:
            break;
        case SettingsPopup::BOOKMARKS:
            settings_popup_default_layout("bookmark popup menu", [&] {
                settings_popup_close_button_ontop();
                text_label_centered(gui, "Bookmarks");
                world.bMan.setup_list_gui();
            });
            break;
        case SettingsPopup::LAYERS:
            settings_popup_default_layout("layer popup menu", [&] {
                settings_popup_close_button_ontop();
                text_label_centered(gui, "Layers");
                world.drawProg.layerMan.listGUI.setup_list_gui();
            });
            break;
        case SettingsPopup::GRIDS:
            settings_popup_default_layout("grid popup menu", [&] {
                settings_popup_close_button_ontop();
                text_label_centered(gui, "Grids");
                world.gridMan.setup_list_gui([&] {
                    settingsPopup = SettingsPopup::NONE;
                });
            });
            break;
        case SettingsPopup::HOST:
            settings_popup_default_layout("host popup menu", [&] {
                settings_popup_close_button_ontop();
                text_label_centered(gui, "Host");
                input_text_field(gui, "lobby", "Lobby", &serverToConnectTo, {
                    .immutable = true
                });
                left_to_right_line_layout(gui, [&] {
                    text_button(gui, "Host button", "Copy & Host", {
                        .wide = true,
                        .onClick = [&] {
                            world.main.input.set_clipboard_str(serverToConnectTo);
                            world.start_hosting(serverToConnectTo, serverLocalID);
                            settingsPopup = SettingsPopup::NONE;
                        }
                    });
#ifdef __ANDROID__
                    text_button(gui, "Share and host button", "Share & Host", {
                        .wide = true,
                        .onClick = [&] {
                            AndroidJNICalls::shareText(serverToConnectTo);
                            main.world->start_hosting(serverToConnectTo, serverLocalID);
                            settingsPopup = SettingsPopup::NONE;
                        }
                    });
#endif
                });
            });
            break;
        case SettingsPopup::LOBBY_INFO:
            settings_popup_default_layout("host lobby info menu", [&] {
                settings_popup_close_button_ontop();
                text_label_centered(gui, "Lobby Info");
                input_text_field(gui, "lobby", "Lobby", &serverToConnectTo, {
                    .immutable = true
                });
                left_to_right_line_layout(gui, [&] {
                    text_button(gui, "Copy button", "Copy", {
                        .wide = true,
                        .onClick = [&] {
                            world.main.input.set_clipboard_str(serverToConnectTo);
                            settingsPopup = SettingsPopup::NONE;
                        }
                    });
#ifdef __ANDROID
                    text_button(gui, "Share button", "Share", {
                        .wide = true,
                        .onClick = [&] {
                            AndroidJNICalls::shareText(serverToConnectTo);
                            settingsPopup = SettingsPopup::NONE;
                        }
                    });
#endif
                });
            });
            break;
        case SettingsPopup::PLAYER_LIST:
            settings_popup_default_layout("player list menu", [&] {
                settings_popup_close_button_ontop();
                text_label_centered(gui, "Player List");
                left_to_right_line_layout(gui, [&]() {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)}
                        },
                        .backgroundColor = convert_vec4<Clay_Color>(SkColor4f{world.ownClientData->get_cursor_color().x(), world.ownClientData->get_cursor_color().y(), world.ownClientData->get_cursor_color().z(), 1.0f}),
                        .cornerRadius = CLAY_CORNER_RADIUS(3)
                    }) {}
                    ellipse_wide_paragraph_label(gui, "own player name", world.ownClientData->get_display_name());
                });
                size_t num = 0;
                for(auto& client : world.clients->get_data()) {
                    if(client != world.ownClientData) {
                        gui.new_id(num++, [&] {
                            left_to_right_line_layout(gui, [&]() {
                                CLAY_AUTO_ID({
                                    .layout = {
                                        .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)}
                                    },
                                    .backgroundColor = convert_vec4<Clay_Color>(SkColor4f{client->get_cursor_color().x(), client->get_cursor_color().y(), client->get_cursor_color().z(), 1.0f}),
                                    .cornerRadius = CLAY_CORNER_RADIUS(3)
                                }) {}
                                ellipse_wide_paragraph_label(gui, "other player name", client->get_display_name());
                                text_button(gui, "teleport button", "Jump To", { .onClick = [&] {
                                    world.drawData.cam.smooth_move_to(world, client->get_cam_coords(), client->get_window_size());
                                    settingsPopup = SettingsPopup::NONE;
                                }});
                            });
                        });
                    }
                }
            });
            break;
        case SettingsPopup::MAIN_SETTINGS:
            if(gui.io.safeWindowRect.width() < 1000.0f) {
                settings_popup_default_layout("main settings top menu", [&] {
                    left_to_right_line_layout(gui, [&] {
                        settingsMenu.top_tabs_layout(main);
                        settings_popup_close_button();
                    });
                    gui.element<ScrollArea>("settings scroll area", ScrollArea::Options{
                        .scrollVertical = true,
                        .clipVertical = true,
                        .scrollbarY = ScrollArea::ScrollbarType::NORMAL,
                        .innerContent = [&] (auto& contentDim) {
                            CLAY_AUTO_ID({
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                    .layoutDirection = CLAY_LEFT_TO_RIGHT
                                },
                            }) {
                                settingsMenu.inner_layout(main, 300.0f);
                            }
                        }
                    });
                }, false, 330.0f);
            }
            else {
                settings_popup_default_layout("main settings side menu", [&] {
                    settings_popup_close_button_ontop();
                    text_label_centered(gui, "Settings");
                    left_to_right_line_layout(gui, [&] {
                        settingsMenu.side_tabs_layout(main);
                        gui.element<ScrollArea>("settings scroll area", ScrollArea::Options{
                            .scrollVertical = true,
                            .clipVertical = true,
                            .scrollbarY = ScrollArea::ScrollbarType::NORMAL,
                            .innerContent = [&] (auto& contentDim) {
                                CLAY_AUTO_ID({
                                    .layout = {
                                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                                    },
                                }) {
                                    settingsMenu.inner_layout(main, 300.0f);
                                }
                            }
                        });
                    });
                }, false, 450.0f);
            }
            break;
    }
}

void CanvasEditorTopToolbar::settings_popup_default_layout(const char* layoutElemId, const std::function<void()>& layoutFunc, bool isRightAttached, float maxWidth) {
    auto& main = s.main;
    auto& gui = main.g.gui;
    auto& io = gui.io;

    Clay_FloatingAttachPointType elementAttach = isRightAttached ? CLAY_ATTACH_POINT_RIGHT_TOP : CLAY_ATTACH_POINT_LEFT_TOP;
    Clay_FloatingAttachPointType parentAttach = isRightAttached ? CLAY_ATTACH_POINT_RIGHT_BOTTOM : CLAY_ATTACH_POINT_LEFT_BOTTOM;

    gui.set_z_index(gui.get_z_index() + 20, [&] {
        gui.element<LayoutElement>(layoutElemId, [&] (LayoutElement*, const Clay_ElementId& lId) {
            Vector2f popupSize = {
                std::clamp(gui.io.safeWindowRect.width() - 10.0f, 20.0f, maxWidth),
                std::clamp(gui.io.safeWindowRect.height() - 40.0f, 50.0f, 600.0f)
            };
            CLAY(lId, {
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0, popupSize.x()), .height = CLAY_SIZING_FIT(0, popupSize.y()) },
                    .padding = CLAY_PADDING_ALL(io.theme->padding1),
                    .childGap = io.theme->childGap1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1),
                .floating = {.offset = {.x = 0, .y = static_cast<float>(io.theme->padding1)}, .zIndex = gui.get_z_index(), .attachPoints = {.element = elementAttach, .parent = parentAttach}, .attachTo = CLAY_ATTACH_TO_PARENT}
            }) {
                layoutFunc();
            }
        });
    });
}

void CanvasEditorTopToolbar::global_log() {
    auto& main = s.main;
    auto& gui = main.g.gui;
    auto& io = gui.io;

    float notificationSize = std::min(240.0f, gui.io.safeWindowRect.width() - 20.0f);

    gui.new_id("Global log popup list phone drawing screen", [&] {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIXED(notificationSize), .height = CLAY_SIZING_FIT(0) },
                .childGap = io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .floating = {.offset = {-10, 10}, .attachPoints = {.element = CLAY_ATTACH_POINT_RIGHT_TOP, .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM}, .attachTo = CLAY_ATTACH_TO_PARENT}
        }) {
            for(size_t i = 0; i < main.logMessages.size(); i++) {
                auto& logM = main.logMessages[i];
                logM.time.update_time_since();
                if(logM.time < UserLogMessage::DISPLAY_TIME) {
                    if(logM.whereToDisplay == UserLogMessage::DISPLAY_DESKTOP_ONLY)
                        continue;
                    float a = 1.0f - lerp_time<float>(logM.time, UserLogMessage::DISPLAY_TIME, UserLogMessage::FADE_START_TIME);
                    gui.new_id(i, [&] {
                        gui.element<LayoutElement>("Global log message", [&] (LayoutElement*, const Clay_ElementId& lId) {
                            CLAY(lId, {
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_FIT(notificationSize), .height = CLAY_SIZING_FIT(0) },
                                    .padding = CLAY_PADDING_ALL(io.theme->padding1),
                                    .childGap = 0,
                                    .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                                },
                                .backgroundColor = convert_vec4<Clay_Color>(color_mul_alpha(io.theme->backColor1, a)),
                                .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1)
                            }) {
                                SkColor4f c{0, 0, 0, 0};
                                switch(logM.color) {
                                    case UserLogMessage::COLOR_NORMAL:
                                        c = io.theme->frontColor1;
                                        break;
                                    case UserLogMessage::COLOR_ERROR:
                                        c = io.theme->errorColor;
                                        break;
                                }
                                text_label_color(gui, logM.text, color_mul_alpha(c, a));
                            }
                        });
                    });
                }
                else
                    break;
            }
        }
    });
}

void CanvasEditorTopToolbar::remaining_area() {
    auto& main = s.main;
    auto& gui = main.g.gui;
    std::vector<IconMenuButton> listOfTopToolbarButtons = {
        IconMenuButton{
            .name = "Undo",
            .svgPath = "data/icons/undo.svg",
            .onClick = [&] {
                main.world->undo_with_checks();
            }
        },
        IconMenuButton{
            .name = "Redo",
            .svgPath = "data/icons/redo.svg",
            .onClick = [&] {
                main.world->redo_with_checks();
            }
        },
        IconMenuButton{
            .name = "Bookmarks",
            .svgPath = "data/icons/bookmark.svg",
            .isSelected = settingsPopup == SettingsPopup::BOOKMARKS,
            .onClick = [&] {
                settingsPopup = settingsPopup == SettingsPopup::BOOKMARKS ? SettingsPopup::NONE : SettingsPopup::BOOKMARKS;
            }
        },
        IconMenuButton{
            .name = "Layers",
            .svgPath = "data/icons/layer.svg",
            .isSelected = settingsPopup == SettingsPopup::LAYERS,
            .onClick = [&] {
                settingsPopup = settingsPopup == SettingsPopup::LAYERS ? SettingsPopup::NONE : SettingsPopup::LAYERS;
            }
        },
        IconMenuButton{
            .name = "Grids",
            .svgPath = "data/icons/grid.svg",
            .isSelected = settingsPopup == SettingsPopup::GRIDS,
            .onClick = [&] {
                settingsPopup = settingsPopup == SettingsPopup::GRIDS ? SettingsPopup::NONE : SettingsPopup::GRIDS;
            }
        },
        IconMenuButton{
            .name = "Hide Toolbars",
            .svgPath = "data/icons/eyeopen.svg",
            .isSelected = false,
            .onClick = [&] {
                main.hideInterface = true;
            }
        },
    };
    gui.element<LayoutElement>("remaining area", [&](LayoutElement* l, const Clay_ElementId& lId) {
        CLAY(lId, {
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
            }
        }) {
            if(l->get_bb().has_value()) {
                float MINIMUM_SPACE_FOR_NAME = s.mobileUI ? 100.0f : 250.0f;
                float spaceForWorldName = l->get_bb().value().width() - BIG_BUTTON_SIZE;
                if(spaceForWorldName < 4.0f)
                    spaceForWorldName = 4.0f;
                size_t buttonFreeSpace;
                for(buttonFreeSpace = 0; buttonFreeSpace < listOfTopToolbarButtons.size(); buttonFreeSpace++) {
                    if(spaceForWorldName <= MINIMUM_SPACE_FOR_NAME + BIG_BUTTON_SIZE)
                        break;
                    spaceForWorldName -= BIG_BUTTON_SIZE;
                }
                if(s.mobileUI)
                    mobile_ui_name_fill();
                else
                    desktop_ui_tabs_fill();
                gui.new_id("visible buttons", [&] {
                    for(size_t i = 0; i < buttonFreeSpace; i++) {
                        gui.new_id(i, [&] {
                            svg_icon_button(gui, "button", listOfTopToolbarButtons.front().svgPath, {
                                .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                .isSelected = listOfTopToolbarButtons.front().isSelected,
                                .onClick = listOfTopToolbarButtons.front().onClick
                            });
                            listOfTopToolbarButtons.erase(listOfTopToolbarButtons.begin());
                        });
                    }
                });
                Element* e = svg_icon_button(gui, "Hidden buttons", "data/icons/RemixIcon/more-2-fill.svg", {
                    .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                    .isSelected = settingsPopup == SettingsPopup::HIDDEN_BUTTONS,
                    .onClick = [&] {
                        settingsPopup = settingsPopup == SettingsPopup::HIDDEN_BUTTONS ? SettingsPopup::NONE : SettingsPopup::HIDDEN_BUTTONS;
                    }
                });
                if(settingsPopup == SettingsPopup::HIDDEN_BUTTONS)
                    hidden_button_popup(e, listOfTopToolbarButtons);
            }
        }
    });
}

void CanvasEditorTopToolbar::mobile_ui_name_fill() {
    auto& main = s.main;
    auto& gui = s.main.g.gui;

    if(main.world->netClient) {
        CLAY_AUTO_ID({
            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0)}}
        }) { }
    }
    else {
        input_text(gui, "world name text", &main.world->name, {
            .decorations = false,
            .onSelect = [&] {
                fileOldPath = main.world->filePath;
            },
            .onDeselect = [&] {
                std::filesystem::remove(fileOldPath);
                std::filesystem::remove(fileOldPath.parent_path() / (fileOldPath.stem().string() + ".jpg"));
                main.world->autosave_to_directory(main.world->filePath.parent_path());
            }
        });
    }
}

void CanvasEditorTopToolbar::desktop_ui_tabs_fill() {
    auto& main = s.main;
    auto& gui = s.main.g.gui;

    std::vector<MovableTabListData::IconNamePair> tabNames;
    for(size_t i = 0; i < main.worlds.size(); i++) {
        auto& w = main.worlds[i];
        bool shouldAddStarNextToName = w->should_ask_before_closing() && !w->netServer && !w->netClient;
        tabNames.emplace_back(w->netObjMan.is_connected() ? "data/icons/network.svg" : "", w->name + (shouldAddStarNextToName ? "*" : ""));
    }

    size_t worldIndex = std::find(main.worlds.begin(), main.worlds.end(), main.world) - main.worlds.begin();

    gui.element<MovableTabList>("file tab list", MovableTabListData{
        .tabNames = tabNames,
        .selectedTab = worldIndex,
        .changeSelectedTab = [&] (size_t i) {
            main.switch_to_tab(i);
        },
        .closeTab = [&] (size_t i) {
            //if(main.worlds[i]->should_ask_before_closing())
            //    add_world_to_close_popup_data(main.worlds[i]);
            //else
            main.set_tab_to_close(main.worlds[i].get());
        }
    });
}

void CanvasEditorTopToolbar::hidden_button_popup(GUIStuff::Element* b, std::vector<IconMenuButton> l) {
    auto& main = s.main;
    auto& gui = main.g.gui;

    l.emplace_back(IconMenuButton{
        .name = "Add Image",
        .svgPath = "data/icons/RemixIcon/image-add-line.svg",
        .onClick = [&] {
            s.open_file_selector("Open File", {{"Any File", "*"}}, [&](const std::filesystem::path& p, const auto& e) {
                CustomEvents::emit_event<CustomEvents::AddFileToCanvasEvent>({
                    .type = CustomEvents::AddFileToCanvasEvent::Type::PATH,
                    .filePath = p,
                    .pos = main.window.size.cast<float>() / 2.0f
                });
            });
        }
    });

    if(s.mobileUI && !main.world->netClient) {
        l.emplace_back(IconMenuButton{
            .name = "Force Save",
            .svgPath = "data/icons/RemixIcon/save-line.svg",
            .onClick = [&] {
                s.save_to_file();
            }
        });
    }

    l.emplace_back(IconMenuButton{
        .name = "Canvas Color",
        .svgPath = "data/icons/RemixIcon/settings-3-line.svg",
        .onClick = [&] {
            //set_settings_menu_popup_mode_with_color_picker_data(SettingsMenuPopup::SETTINGS, &backgroundColorTemporary, {
            //    .onChange = [&] {
            //        main.world->canvasTheme.set_back_color({backgroundColorTemporary.x(), backgroundColorTemporary.y(), backgroundColorTemporary.z()});
            //    }
            //});
        }
    });

    l.emplace_back(IconMenuButton{
        .name = "Paste",
        .svgPath = "data/icons/RemixIcon/clipboard-line.svg",
        .onClick = [&] {
            main.world->drawProg.paste_object_clipboard(main.window.safeArea.center());
        }
    });

    l.emplace_back(IconMenuButton{
        .name = "Export Area",
        .svgPath = "data/icons/RemixIcon/upload-line.svg",
        .onClick = [&] {
            main.world->drawProg.switch_to_tool(DrawingProgramToolType::SCREENSHOT);
        }
    });

    if(!main.world->netClient && !main.world->netServer) {
        l.emplace_back(IconMenuButton{
            .name = "Host",
            .svgPath = "data/icons/network.svg",
            .onClick = [&] {
                serverLocalID = NetLibrary::get_random_server_local_id();
                serverToConnectTo = NetLibrary::get_global_id() + serverLocalID;
                settingsPopup = settingsPopup == SettingsPopup::HOST ? SettingsPopup::NONE : SettingsPopup::HOST;
            }
        });
    }

    if(main.world->netServer) {
        l.emplace_back(IconMenuButton{
            .name = "Lobby Info",
            .svgPath = "data/icons/network.svg",
            .onClick = [&] {
                settingsPopup = settingsPopup == SettingsPopup::LOBBY_INFO ? SettingsPopup::NONE : SettingsPopup::LOBBY_INFO;
            }
        });
    }

    if(main.world->netClient || main.world->netServer) {
        l.emplace_back(IconMenuButton{
            .name = "Player List",
            .svgPath = "data/icons/list.svg",
            .onClick = [&] {
                settingsPopup = settingsPopup == SettingsPopup::PLAYER_LIST ? SettingsPopup::NONE : SettingsPopup::PLAYER_LIST;
            }
        });
    }

    icon_menu_dropdown(b, "top toolbar hidden button popup", gui.get_z_index() + 20, l, [&] { settingsPopup = SettingsPopup::NONE; });
}

void CanvasEditorTopToolbar::icon_menu_dropdown(GUIStuff::Element* b, const char* id, int zIndex, const std::vector<IconMenuButton>& l, const std::function<void()>& clickCallback) {
    auto& main = s.main;
    auto& gui = main.g.gui;

    auto dropdownPosLimits = gui.io.safeWindowRect;
    if(!s.mobileUI)
        dropdownPosLimits.add_padding_LTRB_with_minimum_distance(-gui.io.theme->padding1, 0.0f, -gui.io.theme->padding1, 0.0f, 1.0f);

    gui.set_z_index(zIndex, [&] {
        dropdown_many_element_popup_layout(gui, id, {
            .button = b,
            .clickAwayCallback = clickCallback,
            .entrySize = {150.0f, 30.0f},
            .entryCount = l.size(),
            .posLimits = dropdownPosLimits,
            .entryLayout = [&] (size_t i) {
                auto& listItem = l[i];
                gui.element<SelectableButton>("button", SelectableButton::Data{
                    .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                    .onClick = [&, oC = listItem.onClick, clickCallback] {
                        clickCallback();
                        oC();
                    },
                    .innerContent = [&] (auto&) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                                .childGap = gui.io.theme->childGap1,
                                .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                            }
                        }) {
                            CLAY_AUTO_ID({
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)},
                                }
                            }) {
                                gui.element<SVGIcon>("icon", listItem.svgPath);
                            }
                            mutable_text_label(gui, "text", listItem.name);
                        }
                    }
                });
            }
        });
    });
}

void CanvasEditorTopToolbar::settings_popup_close_button_ontop() {
    auto& gui = s.main.g.gui;
    auto& io = gui.io;

    gui.element<LayoutElement>("settings popup close button layout", [&] (LayoutElement*, const Clay_ElementId& lId) {
        CLAY(lId, {
            .floating = {.offset = {.x = -static_cast<float>(io.theme->padding1), .y = static_cast<float>(io.theme->padding1)}, .zIndex = gui.get_z_index(), .attachPoints = {.element = CLAY_ATTACH_POINT_RIGHT_TOP, .parent = CLAY_ATTACH_POINT_RIGHT_TOP}, .attachTo = CLAY_ATTACH_TO_PARENT}
        }) {
            settings_popup_close_button();
        }
    });
}

void CanvasEditorTopToolbar::settings_popup_close_button() {
    auto& gui = s.main.g.gui;

    svg_icon_button(gui, "settings popup close button", "data/icons/close.svg", {
        .size = SMALL_BUTTON_SIZE,
        .onClick = [&] {
            settingsPopup = SettingsPopup::NONE;
        }
    });
}
