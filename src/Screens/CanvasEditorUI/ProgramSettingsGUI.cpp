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

#include "ProgramSettingsGUI.hpp"
#include "../../MainProgram.hpp"

#include "../../GUIStuff/Elements/LayoutElement.hpp"
#include "../../GUIStuff/Elements/DropDown.hpp"
#include "../../GUIStuff/Elements/ScrollArea.hpp"
#include "../../GUIStuff/ElementHelpers/ColorPickerHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/CheckBoxHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/NumberSliderHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"
#include "../../World.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../../ResourceDisplay/ImageResourceDisplay.hpp"
#include <SDL3/SDL_time.h>
#include <SDL3/SDL_timer.h>

using namespace GUIStuff;
using namespace ElementHelpers;

void ProgramSettingsGUI::top_tabs_layout(MainProgram& main) {
    auto& gui = main.g.gui;


    gui.element<LayoutElement>("program settings top bar", [&](LayoutElement*, const Clay_ElementId& lId) {
        CLAY(lId, {
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(40)},
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
        }) {
            gui.clipping_element<ScrollArea>("settings scroll area", ScrollArea::Options{
                .scrollHorizontal = true,
                .clipHorizontal = true,
                .scrollbarX = ScrollArea::ScrollbarType::NO_INTERACTION,
                .innerContent = [&] (auto&) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                            .layoutDirection = CLAY_LEFT_TO_RIGHT
                        },
                    }) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                .padding = CLAY_PADDING_ALL(1)
                            }
                        }) {
                            tabs_layout(main, false);
                        }
                    }
                }
            });
        }
    });
}

void ProgramSettingsGUI::side_tabs_layout(MainProgram& main) {
    auto& gui = main.g.gui;

    gui.element<LayoutElement>("program settings side bar", [&](LayoutElement*, const Clay_ElementId& lId) {
        CLAY(lId, {
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIT(120), .height = CLAY_SIZING_GROW(0)},
            },
        }) {
            gui.clipping_element<ScrollArea>("settings scroll area", ScrollArea::Options{
                .scrollVertical = true,
                .clipVertical = true,
                .scrollbarY = ScrollArea::ScrollbarType::NO_INTERACTION,
                .innerContent = [&] (auto&) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP },
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        },
                    }) {
                        tabs_layout(main, true);
                    }
                }
            });
        }
    });
}

void ProgramSettingsGUI::tabs_layout(MainProgram& main, bool wideButtons) {
    auto& gui = main.g.gui;

    auto category_button = [&](const char* name, SettingsMenu setting) {
        text_button(gui, name, name, {
            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
            .isSelected = settingsMenu == setting,
            .wide = wideButtons,
            .onClick = [&, setting] { settingsMenu = setting; }
        });
    };

    category_button("General", SettingsMenu::GENERAL);
    category_button("Graphics", SettingsMenu::GRAPHICS);
    category_button("Tablet", SettingsMenu::TABLET);
    category_button("Touch", SettingsMenu::TOUCH);
    category_button("Theme", SettingsMenu::THEME);
    category_button("Keybinds", SettingsMenu::KEYBINDS);
#ifndef __ANDROID__
    category_button("Debug", SettingsMenu::DEBUG);
#endif
}

void ProgramSettingsGUI::inner_layout(MainProgram& main, float maxWidth) {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0, maxWidth), .height = CLAY_SIZING_GROW(0)},
            .childGap = gui.io.theme->childGap1,
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
    }) {
        switch(settingsMenu) {
            case SettingsMenu::GENERAL: {
                input_text_field(gui, "display name input", "Display name", &main.conf.displayName, {
                    .onEdit = [&] {
                        main.update_display_names();
                    }
                });
                color_picker_button_field(gui, "defaultCanvasBackgroundColor", "Default canvas background color", &main.conf.defaultCanvasBackgroundColor, { .hasAlpha = false });
                #ifndef __EMSCRIPTEN__
                    checkbox_boolean_field(gui, "native file pick", "Use native file picker", &main.conf.useNativeFilePicker);
                    checkbox_boolean_field(gui, "update notifications enable", "Check for updates on startup", &main.conf.checkForUpdates);
                #endif
                slider_scalar_field(gui, "drag zoom slider", "Drag zoom speed", &main.conf.dragZoomSpeed, 0.0, 1.0, {.decimalPrecision = 3});
                slider_scalar_field(gui, "scroll zoom slider", "Scroll zoom speed", &main.conf.scrollZoomSpeed, 0.0, 1.0, {.decimalPrecision = 3});
                checkbox_boolean_field(gui, "flip zoom tool direction", "Flip zoom tool direction", &main.conf.flipZoomToolDirection);
                checkbox_boolean_field(gui, "make all tools share same size", "Make all tools share size", &main.toolConfig.globalConf.useGlobalRelativeWidth);
                slider_scalar_field(gui, "brush stroke smoothing factor", "Brush stroke smoothing", &main.conf.brushStrokeSmoothingFactor, 0.3f, 2.0f, {.decimalPrecision = 2});
                input_scalar_field(gui, "jump transition time", "Jump transition time", &main.conf.jumpTransitionTime, 0.01f, 1000.0f, {.decimalPrecision = 2});

                checkbox_boolean_field(gui, "real time eraser", "Eraser works in real time", &main.conf.realTimeEraser);
                checkbox_boolean_field(gui, "force extension on path", "Force extension on path when saving files", &main.conf.forceExtensionOnPath);
                #ifdef LINUX_ADD_VIDEO_DRIVER_OPTION
                    text_label(gui, "Video driver (changes on restart, currently " + main.window.sdlCurrentVideoDriver + "):");
                    radio_button_selector(gui, "Video driver selection", &main.conf.videoDriver, {
                        {"Default", GlobalConfig::VideoDriver::DEFAULT},
                        {"Prefer X11", GlobalConfig::VideoDriver::X11},
                        {"Prefer Wayland", GlobalConfig::VideoDriver::WAYLAND}
                    });
                #endif
                break;
            }
            case SettingsMenu::GRAPHICS: {
                input_scalar_field(gui, "GUI Scale", "GUI Scale", &main.conf.guiScale, 0.5f, 5.0f, {
                    .decimalPrecision = 1,
                    .onEdit = [&] { main.g.window_update(); }
                });
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
                input_scalar_field<unsigned>(gui, "FPS cap", "FPS Cap", &main.conf.mainCallbackRate, 10, 100000, {
                    .onEdit = [&] {
                        main.update_main_loop_call_rate(main.conf.mainCallbackRate);
                    }
                });
                input_scalar_field<unsigned>(gui, "Background FPS cap", "Background FPS Cap", &main.conf.mainCallbackRateBackground, 1, 100000);
                #ifndef __EMSCRIPTEN__
                    checkbox_boolean_field(gui, "disable graphics driver workarounds", "Disable graphics driver workarounds (enabling or disabling this might fix some graphical glitches, requires restart)", &main.conf.disableGraphicsDriverWorkarounds);
                    checkbox_boolean_field(gui, "apply display scale", "Apply display scale", &main.conf.applyDisplayScale);
                #endif
                break;
            }
            case SettingsMenu::TABLET: {
                checkbox_boolean_field(gui, "pen pressure width", "Pen pressure affects brush size", &main.conf.tabletOptions.pressureAffectsBrushWidth);
                input_scalar_field<uint8_t>(gui, "middle click", "Middle click pen button", &main.conf.tabletOptions.middleClickButton, 1, 255);
                input_scalar_field<uint8_t>(gui, "right click", "Right click pen button", &main.conf.tabletOptions.rightClickButton, 1, 255);
                slider_scalar_field(gui, "tablet brush minimum size", "Brush relative minimum size", &main.conf.tabletOptions.brushMinimumSize, 0.0f, 1.0f, {.decimalPrecision = 3});
                slider_scalar_field(gui, "tablet brush pressure smoothing factor", "Brush pressure smoothing factor", &main.conf.tabletOptions.brushPressureSmoothingFactor, 0.0f, 1.0f, {.decimalPrecision = 3});
                checkbox_boolean_field(gui, "tablet zoom with button method", "Zoom when pen touching tablet and pen button assigned to middle click is held", &main.conf.tabletOptions.zoomWhilePenDownAndButtonHeld);
                #ifdef _WIN32
                    checkbox_boolean_field(gui, "mouse ignore when pen proximity", "Ignore mouse movement when pen in proximity", &main.conf.tabletOptions.ignoreMouseMovementWhenPenInProximity);
                #endif
                checkbox_boolean_field(gui, "disable touch when pen in proximity", "Disable touch when pen in proximity", &main.conf.tabletOptions.disableTouchWhenPenInProximity);
                break;
            }
            case SettingsMenu::TOUCH: {
                std::vector<std::pair<std::string_view, GlobalConfig::TouchOptions::SingleFingerAction>> singleFingerOptions = {
                    {"None", GlobalConfig::TouchOptions::SingleFingerAction::NONE},
                    {"Active Tool", GlobalConfig::TouchOptions::SingleFingerAction::ACTIVE_TOOL},
                    {"Pan", GlobalConfig::TouchOptions::SingleFingerAction::PAN},
                    {"Lasso Select", GlobalConfig::TouchOptions::SingleFingerAction::LASSO}
                };
                text_label(gui, "Finger action");
                radio_button_selector(gui, "Finger action options", &main.conf.touchOptions.singleFingerAction, singleFingerOptions);
                text_label(gui, "Two finger move");
                checkbox_boolean_field(gui, "two finger rotation", "Enable canvas rotation", &main.conf.touchOptions.twoFingerRotation);
                checkbox_boolean_field(gui, "two finger rotation snap", "Snap rotation to common angles", &main.conf.touchOptions.twoFingerSnapRotation);
                checkbox_boolean_field(gui, "two finger zoom", "Enable canvas zoom", &main.conf.touchOptions.twoFingerZoom);
                std::vector<std::pair<std::string_view, GlobalConfig::TouchOptions::MultiFingerTapAction>> multiFingerTapOptions = {
                    {"None", GlobalConfig::TouchOptions::MultiFingerTapAction::NONE},
                    {"Undo", GlobalConfig::TouchOptions::MultiFingerTapAction::UNDO},
                    {"Redo", GlobalConfig::TouchOptions::MultiFingerTapAction::REDO},
                    {"Toggle Interface", GlobalConfig::TouchOptions::MultiFingerTapAction::TOGGLE_INTERFACE}
                };
                text_label(gui, "Two finger tap");
                radio_button_selector(gui, "Two finger tap options", &main.conf.touchOptions.twoFingerTapAction, multiFingerTapOptions);
                text_label(gui, "Three finger tap");
                radio_button_selector(gui, "Three finger tap options", &main.conf.touchOptions.threeFingerTapAction, multiFingerTapOptions);
                text_label(gui, "Four finger tap");
                radio_button_selector(gui, "Four finger tap options", &main.conf.touchOptions.fourFingerTapAction, multiFingerTapOptions);
                break;
            }
            case SettingsMenu::THEME: {
                if(!themeData.selectedThemeIndex)
                    reload_theme_list(main);

                CLAY_AUTO_ID({.layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0)},
                    .childGap = io.theme->childGap1,
                    .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                }
                }) {
                    text_label(gui, "Theme: ");
                    gui.element<DropDown<size_t>>("dropdownSelectThemes", &themeData.selectedThemeIndex.value(), themeData.themeDirList, DropdownOptions{
                        .onClick = [&]() {
                            main.conf.themeCurrentlyLoaded = themeData.themeDirList[themeData.selectedThemeIndex.value()];
                            reload_theme_list(main);
                        }
                    });
                }
                left_to_right_line_layout(gui, [&]() {
                    if(themeData.selectedThemeIndex != 0) {
                        text_button_wide(gui, "savethemebutton", "Save", [&] {
                            main.conf.themeCurrentlyLoaded = themeData.themeDirList[themeData.selectedThemeIndex.value()];
                            main.g.save_theme(main.conf.configPath, main.conf.themeCurrentlyLoaded);
                            reload_theme_list(main);
                        });
                    }
                    text_button_wide(gui, "saveasthemebutton", "Save As", [&]() {
                        themeData.openedSaveAsMenu = !themeData.openedSaveAsMenu;
                    });
                    text_button_wide(gui, "reloadthemebutton", "Reload", [&] {
                        main.conf.themeCurrentlyLoaded = themeData.themeDirList[themeData.selectedThemeIndex.value()];
                        reload_theme_list(main);
                    });
                    if(themeData.selectedThemeIndex != 0) {
                        text_button_wide(gui, "deletethemebutton", "Delete", [&] {
                            try { std::filesystem::remove(main.conf.configPath / "themes" / (themeData.themeDirList[themeData.selectedThemeIndex.value()] + ".json")); } catch(...) { }
                            main.conf.themeCurrentlyLoaded = "Default";
                            reload_theme_list(main);
                        });
                    }
                });
                if(themeData.openedSaveAsMenu) {
                    input_text_field(gui, "Theme name:", "Theme name: ", &main.conf.themeCurrentlyLoaded);
                    left_to_right_line_layout(gui, [&]() {
                        text_button_wide(gui, "saveasdone", "Done", [&] {
                            main.g.save_theme(main.conf.configPath, main.conf.themeCurrentlyLoaded);
                            reload_theme_list(main);
                        });
                        text_button_wide(gui, "saveascancel", "Cancel", [&] {
                            themeData.openedSaveAsMenu = false;
                        });
                    });
                }
                text_label(gui, "Edit theme:");
                text_label_light(gui, "Note: Changes only remain if theme is saved");
                auto theme_color_field = [&](const char* id, const char* name, SkColor4f* c) {
                    color_picker_button_field<SkColor4f>(gui, id, name, c, {});
                };
                theme_color_field("fillColor1", "Fill Color 1", &io.theme->fillColor1);
                theme_color_field("fillColor2", "Fill Color 2", &io.theme->fillColor2);
                theme_color_field("backColor0", "Back Color 0", &io.theme->backColor0);
                theme_color_field("backColor1", "Back Color 1", &io.theme->backColor1);
                theme_color_field("backColor2", "Back Color 2", &io.theme->backColor2);
                theme_color_field("frontColor1", "Front Color 1", &io.theme->frontColor1);
                theme_color_field("frontColor2", "Front Color 2", &io.theme->frontColor2);
                theme_color_field("composeColor1", "Compose Color", &io.theme->composeColor);
                theme_color_field("warningColor", "Warning Color", &io.theme->warningColor);
                theme_color_field("errorColor", "Error Color", &io.theme->errorColor);
                //gui.slider_scalar_field("hoverExpandTime", "Hover Expand Time", &io.theme->hoverExpandTime, 0.001f, 1.0f);
                input_scalar_field<uint16_t>(gui, "childGap1", "Gap between child elements", &io.theme->childGap1, 0, 30);
                input_scalar_field<uint16_t>(gui, "padding1", "Window padding", &io.theme->padding1, 0, 30);
                slider_scalar_field<float>(gui, "windowCorners1", "Window corner radius", &io.theme->windowCorners1, 0, 30);
                break;
            }
            case SettingsMenu::KEYBINDS: {
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
                                .isSelected = main.keybindWaiting.has_value() && main.keybindWaiting.value() == i,
                                .wide = true,
                                .onClick = [&, i] {
                                    main.keybindWaiting = i;
                                }
                            });
                        }
                    });
                }
                break;
            }
            case SettingsMenu::DEBUG: {
                #ifndef __EMSCRIPTEN__
                    checkbox_boolean_field(gui, "use mobile UI", "Use mobile UI (requires restart)", &main.conf.mobileUI);
                #endif
                input_scalars_field(gui, "jump transition easing", "Jump easing", &main.conf.jumpTransitionEasing, 4, -10.0f, 10.0f, { .decimalPrecision = 2 });
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
                break;
            }
        }
    }
}

void ProgramSettingsGUI::text_button_wide(GUIManager& gui, const char* id, const char* str, const std::function<void()>& onClick) {
    text_button(gui, id, str, {
        .wide = true,
        .onClick = onClick
    });
}

void ProgramSettingsGUI::reload_theme_list(MainProgram& main) {
    themeData.selectedThemeIndex = 0;

    themeData.themeDirList.clear();
    themeData.themeDirList.emplace_back("Default");
    std::filesystem::path themeDir = main.conf.configPath / "themes";
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
    if(!main.g.load_theme(main.conf.configPath, main.conf.themeCurrentlyLoaded))
        themeData.selectedThemeIndex = 0;

    themeData.openedSaveAsMenu = false;
}
