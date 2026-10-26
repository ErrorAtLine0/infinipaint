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

#include "CanvasEditorScreen.hpp"
#include "../MainProgram.hpp"
#include "CanvasEditorUI/CanvasEditorCenterPopupGUI.hpp"
#include "CanvasScreen.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIStuff/Elements/GridScrollArea.hpp"
#include "../GUIStuff/Elements/DropDown.hpp"
#include "../GUIStuff/Elements/ColorPicker.hpp"
#include "../GUIStuff/Elements/TreeListing.hpp"
#include "../GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "../GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../GUIStuff/ElementHelpers/TextBoxHelpers.hpp"
#include "../GUIStuff/Elements/RotateWheel.hpp"
#include "FileSelectScreen.hpp"
#include "../NetThreadManager.hpp"
#include <Helpers/Logger.hpp>
#ifdef __EMSCRIPTEN__
#include "EmscriptenHelpers/emscripten_browser_file.h"
#endif

using namespace GUIStuff;
using namespace ElementHelpers;

CanvasEditorScreen::CanvasEditorScreen(MainProgram& m, bool initMobileUI, const std::vector<std::shared_ptr<World>>& worldInit):
    CanvasScreen(m),
    centerPopup(*this),
    topToolbar(*this),
    worlds(worldInit),
    mobileUI(initMobileUI)
{
    world = worlds.front();
}

void CanvasEditorScreen::update() {
    std::erase_if(main.logMessages, [&](auto& logM) {
        logM.time.update_time_since();
        if(logM.time > UserLogMessage::FADE_START_TIME) {
            main.g.gui.set_to_layout();
            return logM.time >= UserLogMessage::DISPLAY_TIME;
        }
        return false;
    });
    CanvasScreen::update();
}

void CanvasEditorScreen::background_update() {
    for(auto& w : worlds) {
        if(!w->update())
            set_tab_to_close(w.get());
    }
}

void CanvasEditorScreen::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    switch(key.key) {
        case InputManager::KEY_TOGGLE_INTERFACE: {
            if(key.down && !key.repeat) {
                main.hideInterface = !main.hideInterface;
                main.g.gui.set_to_layout();
            }
            break;
        }
        case InputManager::KEY_SAVE: {
            if(key.down && !key.repeat) {
                if(mobileUI)
                    mobile_save_file();
                else
                    desktop_save_file();
            }
            break;
        }
        case InputManager::KEY_SAVE_AS: {
            if(key.down && !key.repeat && !mobileUI)
                desktop_save_as_file();
            break;
        }
        case InputManager::KEY_OPEN_CHAT: {
            if(key.down && !key.repeat) {
            }
            break;
        }
        case InputManager::KEY_SHOW_PLAYER_LIST: {
            if(key.down && !key.repeat)
                topToolbar.toggle_player_list(main);
            break;
        }
    }
    CanvasScreen::input_key_callback(key);
}

void CanvasEditorScreen::center_message(const char* id, const std::string& m) {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    gui.set_z_index(gui.get_z_index() + 1, [&] {
        gui.element<LayoutElement>(id, [&] (LayoutElement*, const Clay_ElementId& lId) {
            CLAY(lId, {
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(io.theme->padding1),
                    .childGap = io.theme->childGap1,
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1),
                .floating = {
                    .zIndex = gui.get_z_index(),
                    .attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER},
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                }
            }) {
                text_label(gui, m);
            }
        });
    });
}

void CanvasEditorScreen::gui_layout_run() {
    main_display();
}

void CanvasEditorScreen::main_display() {
    centerPopup.layout();
    CLAY_AUTO_ID({
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
            .childGap = main.g.gui.io.theme->childGap1,
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
    }) {
        if(world->netClient && world->clientStillConnecting) {
            topToolbar.layout_top_toolbar();
            center_message("Connecting to server message", "Connecting to server...");
        }
        else {
            if(main.hideInterface)
                hidden_gui();
            else {
                topToolbar.layout_top_toolbar();
                CLAY_AUTO_ID({
                    .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
                }) {}
                bottom_toolbar();
            }
            if(!world->drawProg.layerMan.is_a_layer_being_edited())
                center_message("Select layer to edit message", "Select a layer to edit");
        }
    }
}

void CanvasEditorScreen::hidden_gui() {
    auto& gui = main.g.gui;

    window_fill_side_bar(gui, WindowFillSideBarConfig{
        .dir = WindowFillSideBarConfig::Direction::TOP
    }, [&] {
        CLAY_AUTO_ID({
            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0)}}
        });
        svg_icon_button(gui, "show gui button", "data/icons/eyeopen.svg", {
            .isSelected = true,
            .onClick = [&] {
                main.hideInterface = false;
            }
        });
    });
}

void CanvasEditorScreen::bottom_toolbar() {
    auto& gui = main.g.gui;
    auto& io = gui.io;
    window_fill_side_bar(gui, {
        .dir = WindowFillSideBarConfig::Direction::BOTTOM,
    }, [&] {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                .padding = CLAY_PADDING_ALL(static_cast<uint16_t>(mobileUI ? 0 : io.theme->padding1)),
                .childGap = io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
            }
        }) {
            if(world->drawProg.phone_gui_tool_specific_bottom_toolbar_exists())
                world->drawProg.phone_gui_tool_specific_bottom_toolbar(*this);
            else
                default_bottom_toolbar();
        }
    });
}

void CanvasEditorScreen::default_bottom_toolbar() {
    auto& gui = main.g.gui;
    auto& io = gui.io;
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0) },
            .childGap = io.theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        }
    }) {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_BOTTOM},
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            }
        }) {
            switch(settingsMenuPopup) {
                case SettingsMenuPopup::NONE:
                    set_settings_menu_popup_mode_with_color_picker_data(SettingsMenuPopup::NONE);
                    break;
                case SettingsMenuPopup::SETTINGS:
                    if(colorPickerPtr != &backgroundColorTemporary)
                        colorPickerPtr = world->drawProg.color_picker_color(colorPickerPtr); // Shouldnt use set_color_picker_data here, it'll reset callbacks
                                                                                                  // This function is only used to make sure the color being pointed to still exists
                    if(colorPickerPtr) {
                        if(colorPickerPopupData.screenType == ColorPickerPopupData::ScreenType::NORMAL) {
                            CLAY_AUTO_ID({
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                }
                            }) {}
                        }
                        color_settings_popup(colorPickerPtr, colorPickerData, [&] {
                            set_color_picker_data(nullptr, ColorSelectorData());
                        });
                    }
                    else {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                            }
                        }) {}
                        reset_color_picker_popup_data();
                        tool_settings_popup();
                    }
                    break;
                case SettingsMenuPopup::COLOR_CHANGE:
                    if(colorPickerPopupData.screenType == ColorPickerPopupData::ScreenType::NORMAL) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                            }
                        }) {}
                    }
                    color_settings_popup(colorPickerPtr, colorPickerData, nullptr);
                    break;
            }
        }

        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                .childGap = io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            }
        }) {
            gui.element<LayoutElement>("bottom toolbar", [&](LayoutElement*, const Clay_ElementId& lId) {
                CLAY(lId, {
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0)}
                    },
                    .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
                    .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1)
                }) {
                    gui.clipping_element<ScrollArea>("tools scroll", ScrollArea::Options{
                        .scrollHorizontal = true,
                        .clipHorizontal = true,
                        .scrollbarX = ScrollArea::ScrollbarType::NONE,
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .xAlign = CLAY_ALIGN_X_LEFT,
                        .yAlign = CLAY_ALIGN_Y_CENTER,
                        .innerContent = [&](auto&) {
                            bottom_toolbar_gui();
                        }
                    });
                }
            });
            gui.element<LayoutElement>("bottom extra toolbar", [&](LayoutElement*, const Clay_ElementId& lId) {
                CLAY(lId, {
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0)},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                    },
                    .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
                    .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1)
                }) {
                    bottom_extra_toolbar_gui();
                }
            });
        }
    }
}

void CanvasEditorScreen::bottom_toolbar_gui() {
    GUIManager& gui = main.g.gui;
    auto& drawP = world->drawProg;

    auto tool_button = [&](const char* id, const std::string& svgPath, DrawingProgramToolType toolType) {
        svg_icon_button(gui, id, svgPath, {
            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
            .isSelected = drawP.drawTool->get_type() == toolType,
            .onClick = [&, toolType] {
                if(toolType == DrawingProgramToolType::EDIT)
                    set_settings_menu_popup_mode_with_color_picker_data(SettingsMenuPopup::SETTINGS);
                drawP.switch_to_tool(toolType);
            }
        });
    };

    tool_button("Brush Toolbar Button", "data/icons/brush.svg", DrawingProgramToolType::BRUSH);
    tool_button("Eraser Toolbar Button", "data/icons/eraser.svg", DrawingProgramToolType::ERASER);
    tool_button("Line Toolbar Button", "data/icons/line.svg", DrawingProgramToolType::LINE);
    tool_button("Text Toolbar Button", "data/icons/text.svg", DrawingProgramToolType::TEXTBOX);
    tool_button("Ellipse Toolbar Button", "data/icons/circle.svg", DrawingProgramToolType::ELLIPSE);
    tool_button("Rect Toolbar Button", "data/icons/rectangle.svg", DrawingProgramToolType::RECTANGLE);
    tool_button("RectSelect Toolbar Button", "data/icons/rectselect.svg", DrawingProgramToolType::RECTSELECT);
    tool_button("LassoSelect Toolbar Button", "data/icons/lassoselect.svg", DrawingProgramToolType::LASSOSELECT);
    tool_button("Edit Toolbar Button", "data/icons/cursor.svg", DrawingProgramToolType::EDIT);
    tool_button("Eyedropper Toolbar Button", "data/icons/eyedropper.svg", DrawingProgramToolType::EYEDROPPER);
    tool_button("Zoom Canvas Toolbar Button", "data/icons/zoom.svg", DrawingProgramToolType::ZOOM);
    tool_button("Pan Canvas Toolbar Button", "data/icons/hand.svg", DrawingProgramToolType::PAN);
}

void CanvasEditorScreen::tool_settings_popup() {
    auto& drawP = world->drawProg;
    auto& gui = main.g.gui;
    auto& io = gui.io;
 
    gui.element<LayoutElement>("tool settings popup", [&] (LayoutElement*, const Clay_ElementId& lId) {
        CLAY(lId, {
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0)},
            },
            .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
            .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1)
        }) {
            gui.clipping_element<ScrollArea>("toolbox scroll area", ScrollArea::Options{
                .scrollVertical = true,
                .clipVertical = true,
                .scrollbarY = ScrollArea::ScrollbarType::NORMAL,
                .innerContent = [&](auto&) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .padding = CLAY_PADDING_ALL(io.theme->padding1),
                            .childGap = io.theme->childGap1,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        },
                    }) {
                        drawP.drawTool->gui_phone_toolbox(*this);
                    }
                }
            });
        }
    });
}

void CanvasEditorScreen::reset_color_picker_popup_data() {
    colorPickerPopupData.screenType = ColorPickerPopupData::ScreenType::NORMAL;
    colorPickerPopupData.newPaletteStr.clear();
    colorPickerPopupData.paletteListSelection.clear();
}

void CanvasEditorScreen::color_settings_popup(Vector4f* color, const ColorSelectorData& d, std::function<void()> backOnPaletteButton, bool setAlphaToOne, bool setToTransparentCancelButton) {
    auto& gui = main.g.gui;
    auto& palette = main.conf.palettes[colorPickerPopupData.selectedPalette];

    int extraColorButtonCount = 1;
    if(colorPickerPopupData.screenType == ColorPickerPopupData::ScreenType::NORMAL) {
        if(backOnPaletteButton)
            extraColorButtonCount++;
        if(setToTransparentCancelButton)
            extraColorButtonCount++;
    }
    else if(colorPickerPopupData.screenType == ColorPickerPopupData::ScreenType::EXTRA && colorPickerPopupData.selectedPalette != 0)
        extraColorButtonCount = 3;

    auto paletteButtonChange = [d] () {
        if(d.onSelect) d.onSelect();
        if(d.onChange) d.onChange();
        if(d.onDeselect) d.onDeselect();
    };

    auto paletteColorPickerGridColorButton = [&](size_t i){
        auto newC = std::make_shared<Vector3f>(palette.colors[i].x(), palette.colors[i].y(), palette.colors[i].z());
        color_button(gui, "c", newC.get(), {
            .isSelected = newC->x() == color->x() && newC->y() == color->y() && newC->z() == color->z(),
            .hasAlpha = false,
            .onClick = [setAlphaToOne, newC, color, paletteButtonChange] {
                // We want to keep the old color's alpha
                color->x() = newC->x();
                color->y() = newC->y();
                color->z() = newC->z();
                if(setAlphaToOne)
                    color->w() = 1.0f;
                paletteButtonChange();
            }
        });
    };

    auto paletteColorPickerGrid = [&] {
        gui.element<GridScrollArea>("color selector grid", GridScrollArea::Options{
            .entryWidth = BIG_BUTTON_SIZE,
            .childAlignmentX = CLAY_ALIGN_X_CENTER,
            .entryHeight = BIG_BUTTON_SIZE,
            .entryCount = palette.colors.size() + extraColorButtonCount,
            .scrollbar = ScrollArea::ScrollbarType::NORMAL,
            .elementContent = [&](size_t i) {
                if(colorPickerPopupData.screenType == ColorPickerPopupData::ScreenType::NORMAL) {
                    int32_t newI = static_cast<int32_t>(i);
                    if(backOnPaletteButton)
                        newI--;
                    if(newI == -1)
                        svg_icon_button(gui, "back on palette button", "data/icons/RemixIcon/arrow-left-s-line.svg", {
                            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                            .onClick = backOnPaletteButton
                        });
                    else if(newI < static_cast<int32_t>(palette.colors.size()))
                        paletteColorPickerGridColorButton(newI);
                    else {
                        if(newI == static_cast<int32_t>(palette.colors.size()) && setToTransparentCancelButton)
                            svg_icon_button(gui, "set to transparent button", "data/icons/close.svg", {
                                .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                .onClick = [&, color, paletteButtonChange] {
                                    color->x() = 0.0f;
                                    color->y() = 0.0f;
                                    color->z() = 0.0f;
                                    color->w() = 0.0f;
                                    paletteButtonChange();
                                }
                            });
                        else
                            svg_icon_button(gui, "extra color settings", "data/icons/RemixIcon/more-fill.svg", {
                                .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                .onClick = [&] {
                                    colorPickerPopupData.screenType = ColorPickerPopupData::ScreenType::EXTRA;
                                }
                            });
                    }
                }
                else {
                    if(i == 0) {
                        svg_icon_button(gui, "extra color settings back button", "data/icons/RemixIcon/arrow-left-s-line.svg", {
                            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                            .onClick = [&] {
                                colorPickerPopupData.screenType = ColorPickerPopupData::ScreenType::NORMAL;
                            }
                        });
                    }
                    else if((i - 1) < palette.colors.size())
                        paletteColorPickerGridColorButton(i - 1);
                    else if(colorPickerPopupData.selectedPalette != 0) {
                        switch(i - palette.colors.size() - 1) {
                            case 0:
                                svg_icon_button(gui, "addcolor", "data/icons/plus.svg", {
                                    .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                    .onClick = [&, color] {
                                        std::erase(palette.colors, Vector3f{color->x(), color->y(), color->z()});
                                        palette.colors.emplace_back(color->x(), color->y(), color->z());
                                    }
                                });
                                break;
                            case 1:
                                svg_icon_button(gui, "deletecolor", "data/icons/close.svg", {
                                    .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                    .onClick = [&, color] {
                                        std::erase(palette.colors, Vector3f{color->x(), color->y(), color->z()});
                                    }
                                });
                                break;
                        }
                    }
                }
            }
        });
    };

    auto paletteColorPickerExtras = [&, d] {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIT(200), .height = CLAY_SIZING_GROW(0)},
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            },
            .aspectRatio = {1.0f}
        }) {
            gui.element<ColorPicker<Vector4f>>("color picker element", color, true, ColorPickerData{
                .onChange = d.onChange,
                .onHold = d.onSelect,
                .onRelease = [&, d] {
                    if(d.onDeselect) d.onDeselect();
                    main.g.gui.set_to_layout();
                }
            });
        }
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                .childGap = gui.io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            }
        }) {
            std::vector<std::string> paletteNames;
            for(auto& p : main.conf.palettes)
                paletteNames.emplace_back(p.name);
            gui.element<DropDown<size_t>>("paletteselector", &colorPickerPopupData.selectedPalette, paletteNames);
            svg_icon_button(gui, "paletteadd", "data/icons/pencil.svg", {
                .onClick = [&] {
                    reset_color_picker_popup_data();
                    colorPickerPopupData.screenType = ColorPickerPopupData::ScreenType::PALETTES;
                }
            });
        }
    };

    auto paletteGUI = [&] {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                .childGap = gui.io.theme->childGap1,
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }
        }) {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                    .childGap = gui.io.theme->childGap1,
                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT
                }
            }) {
                svg_icon_button(gui, "palette back", "data/icons/RemixIcon/arrow-left-s-line.svg", {
                    .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                    .onClick = [&] {
                        colorPickerPopupData.screenType = ColorPickerPopupData::ScreenType::EXTRA;
                    }
                });
                text_label(gui, "Palettes");
            }

            gui.element<TreeListing>("palette list", TreeListing::Data{
                .selectedIndices = &colorPickerPopupData.paletteListSelection,
                .dirInfo = [&](const TreeListingObjIndexList& objIndex) {
                    std::optional<TreeListing::DirectoryInfo> d;
                    if(objIndex.empty()) {
                        d = TreeListing::DirectoryInfo();
                        d.value().isOpen = true;
                        d.value().dirSize = main.conf.palettes.size();
                    }
                    return d;
                },
                .drawNonDirectoryObjIconGUI = [&](const TreeListingObjIndexList& objIndex) {},
                .drawObjGUI = [&](const TreeListingObjIndexList& objIndex) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                        }
                    }) {
                        text_label(gui, main.conf.palettes[objIndex.back()].name);
                    }
                    if(objIndex.back() != 0) {
                        gui.set_z_index_keep_clipping_region(gui.get_z_index() + 1, [&] {
                            svg_icon_button(gui, "delete button", "data/icons/trash.svg", {
                                .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                .size = TreeListing::ENTRY_HEIGHT,
                                .onClick = [&, objIndex] {
                                    colorPickerPopupData.selectedPalette = 0;
                                    main.conf.palettes.erase(main.conf.palettes.begin() + objIndex.back());
                                }
                            });
                        });
                    }
                },
                .moveObj = [&](const std::vector<TreeListingObjIndexList>& objIndices, const TreeListingObjIndexList& newObjIndex) {
                    if(newObjIndex.back() == 0)
                        return;
                    std::deque<GlobalConfig::Palette> movedPalettes;
                    size_t moveIndex = newObjIndex.back();
                    for(auto& p : objIndices | std::views::reverse) {
                        if(p.back() != 0) {
                            if(moveIndex > p.back())
                                moveIndex--;
                            movedPalettes.emplace_front(main.conf.palettes[p.back()]);
                            main.conf.palettes.erase(main.conf.palettes.begin() + p.back());
                        }
                    }
                    colorPickerPopupData.selectedPalette = 0;
                    main.conf.palettes.insert(main.conf.palettes.begin() + moveIndex, movedPalettes.begin(), movedPalettes.end());
                }
            });
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                    .childGap = gui.io.theme->childGap1,
                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT
                }
            }) {
                auto addPaletteFunc = [&] {
                    if(!colorPickerPopupData.newPaletteStr.empty()) {
                        main.conf.palettes.emplace_back();
                        main.conf.palettes.back().name = colorPickerPopupData.newPaletteStr;
                        colorPickerPopupData.selectedPalette = main.conf.palettes.size() - 1;
                        colorPickerPopupData.screenType = ColorPickerPopupData::ScreenType::EXTRA;
                        gui.set_to_layout();
                    }
                };
                input_text(gui, "paletteinputname", &colorPickerPopupData.newPaletteStr, {
                    .emptyText = "New Palette Name...",
                    .onEnter = addPaletteFunc
                });
                svg_icon_button(gui, "paletteadd", "data/icons/plus.svg", {
                    .onClick = addPaletteFunc
                });
            }
        }
    };

    gui.element<LayoutElement>("color settings popup", [&] (LayoutElement* l, const Clay_ElementId& lId) {
        CLAY(lId, {
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                .childGap = gui.io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            },
            .backgroundColor = convert_vec4<Clay_Color>(gui.io.theme->backColor1),
            .cornerRadius = CLAY_CORNER_RADIUS(gui.io.theme->windowCorners1)
        }) {
            switch(colorPickerPopupData.screenType) {
                case ColorPickerPopupData::ScreenType::NORMAL:
                    paletteColorPickerGrid();
                    break;
                case ColorPickerPopupData::ScreenType::EXTRA:
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .childGap = gui.io.theme->childGap1,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        },
                    }) {
                        paletteColorPickerGrid();
                    }
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0)},
                            .childGap = gui.io.theme->childGap1,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        },
                    }) {
                        paletteColorPickerExtras();
                    }
                    break;
                case ColorPickerPopupData::ScreenType::PALETTES:
                    paletteGUI();
                    break;
            }
        }
    });
}

void CanvasEditorScreen::bottom_extra_toolbar_gui() {
    GUIManager& gui = main.g.gui;

    if(main.conf.guiOptions.showToolbarRotateWheel) {
        CLAY_AUTO_ID({
            .layout = {.sizing = {.width = CLAY_SIZING_FIXED(BIG_BUTTON_SIZE), .height = CLAY_SIZING_FIXED(BIG_BUTTON_SIZE)}}
        }) {
            std::shared_ptr<double> newRotationAngle = std::make_shared<double>(world->drawData.cam.c.rotation);
            gui.element<RotateWheel>("Canvas Rotate Wheel", newRotationAngle.get(), [&, newRotationAngle] {
                world->drawData.cam.c.rotate_about(world->drawData.cam.c.from_space(world->main.window.size.cast<float>() * 0.5f), *newRotationAngle - world->drawData.cam.c.rotation);
                *newRotationAngle = world->drawData.cam.c.rotation;
                world->drawData.cam.lastRotationTime.update_time_point();
            });
        }
    }

    svg_icon_button(gui, "tool settings", "data/icons/RemixIcon/settings-3-line.svg", {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = settingsMenuPopup == SettingsMenuPopup::SETTINGS,
        .onClick = [&] {
            set_settings_menu_popup_mode_with_color_picker_data((settingsMenuPopup == SettingsMenuPopup::SETTINGS) ? SettingsMenuPopup::NONE : SettingsMenuPopup::SETTINGS);
        }
    });

    color_button(gui, "foreground color", &main.toolConfig.globalConf.foregroundColor, {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = settingsMenuPopup == SettingsMenuPopup::COLOR_CHANGE && colorPickerPtr == &main.toolConfig.globalConf.foregroundColor,
        .hasAlpha = true,
        .onClick = [&] {
            if(settingsMenuPopup == SettingsMenuPopup::COLOR_CHANGE && colorPickerPtr == &main.toolConfig.globalConf.foregroundColor)
                set_settings_menu_popup_mode_with_color_picker_data(SettingsMenuPopup::NONE);
            else
                set_settings_menu_popup_mode_with_color_picker_data(SettingsMenuPopup::COLOR_CHANGE, &main.toolConfig.globalConf.foregroundColor);
        }
    });

    color_button(gui, "background color", &main.toolConfig.globalConf.backgroundColor, {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = settingsMenuPopup == SettingsMenuPopup::COLOR_CHANGE && colorPickerPtr == &main.toolConfig.globalConf.backgroundColor,
        .hasAlpha = true,
        .onClick = [&] {
            if(settingsMenuPopup == SettingsMenuPopup::COLOR_CHANGE && colorPickerPtr == &main.toolConfig.globalConf.backgroundColor)
                set_settings_menu_popup_mode_with_color_picker_data(SettingsMenuPopup::NONE, nullptr);
            else
                set_settings_menu_popup_mode_with_color_picker_data(SettingsMenuPopup::COLOR_CHANGE, &main.toolConfig.globalConf.backgroundColor);
        }
    });
}

void CanvasEditorScreen::input_global_back_button_callback() {
    if(main.hideInterface) {
        main.hideInterface = false;
        main.g.gui.set_to_layout();
    }
    else if(mobileUI) {
        mobile_save_file();
        main.set_screen([&] (std::unique_ptr<Screen>) { return std::make_unique<FileSelectScreen>(main); });
        main.g.gui.set_to_layout();
    }
}

bool CanvasEditorScreen::is_using_network() {
    bool isAnyWorldConnected = false;
    for(auto& w : worlds)
        isAnyWorldConnected |= w->is_connected();
    return isAnyWorldConnected;
}

void CanvasEditorScreen::open_world_file(bool isClient, const std::string& netSource, const std::string& serverLocalID2) {
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
            CustomEvents::emit_event<CustomEvents::OpenInfiniPaintFileEvent>({
                .isClient = uD->iC,
                .filePathSource = std::filesystem::path(fileName),
                .netSource = uD->nS,
                .serverLocalID = uD->sLID,
                .fileDataBuffer = std::string(buffer)
            });
        }
    }, &uploadData);
#else
    open_file_selector("Open", {{"InfiniPaint Canvas", World::FILE_EXTENSION}, {"Any File", "*"}}, [&, isClient = isClient, netSource = netSource, serverLocalID2 = serverLocalID2](const std::filesystem::path& p, const auto& e) {
        CustomEvents::emit_event<CustomEvents::OpenInfiniPaintFileEvent>({
            .isClient = isClient,
            .filePathSource = p,
            .netSource = netSource,
            .serverLocalID = serverLocalID2
        });
    });
#endif
}

void CanvasEditorScreen::desktop_save_file() {
    if(world->filePath == std::filesystem::path())
        desktop_save_as_file();
    else
        world->save_to_file(world->filePath);
}

void CanvasEditorScreen::desktop_save_as_file() {
    #ifdef __EMSCRIPTEN__
        centerPopup.set_download_file_popup();
    #else
        open_file_selector("Save", {{"InfiniPaint Canvas", World::FILE_EXTENSION}}, [w = make_weak_ptr(world)](const std::filesystem::path& p, const auto& e) {
            auto world = w.lock();
            if(world)
                world->save_to_file(p);
        }, "", true);
    #endif
}

void CanvasEditorScreen::mobile_save_file() {
    if(world && !world->netClient)
        world->save_to_file(world->filePath);
}

bool CanvasEditorScreen::input_keybind_callback(const Vector2ui32& newKey) {
    return topToolbar.input_keybind_callback(main, newKey);
}

void CanvasEditorScreen::input_window_resize_callback(const InputManager::WindowResizeCallbackArgs& w) {
    for(auto& wor : worlds)
        wor->drawData.cam.set_viewing_area(main.window.size.cast<float>());
}

void CanvasEditorScreen::post_callback() {
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
        if(mobileUI)
            main.set_screen([&] (std::unique_ptr<Screen>) { return std::make_unique<FileSelectScreen>(main); });
        else {
            if(worlds.empty())
                create_new_tab({
                    .isClient = false
                });
            else if(!world)
                switch_to_tab(0);
        }
        main.g.gui.set_to_layout();
    }
}

void CanvasEditorScreen::input_app_about_to_go_to_background_callback() {
    if(mobileUI)
        mobile_save_file();
}

bool CanvasEditorScreen::app_close_requested() {
    return centerPopup.app_close_requested();
}

void CanvasEditorScreen::color_selector(Vector4f* color, const ColorSelectorData& colorSelectorData) {
    if(colorPickerPtr != color)
        set_color_picker_data(color, colorSelectorData);
    else
        set_color_picker_data(nullptr, ColorSelectorData());
    main.g.gui.set_to_layout();
}

void CanvasEditorScreen::input_drop_file_callback(const InputManager::DropCallbackArgs& drop) {
    if(!mobileUI && std::filesystem::is_regular_file(drop.data)) {
        std::filesystem::path droppedFilePath(drop.data);
        if(droppedFilePath.has_extension() && droppedFilePath.extension().string() == World::DOT_FILE_EXTENSION) {
            CustomEvents::emit_event<CustomEvents::OpenInfiniPaintFileEvent>({
                .isClient = false,
                .filePathSource = droppedFilePath
            });
            return;
        }
    }
    world->input_drop_file_callback(drop);
}

void CanvasEditorScreen::color_selector_button(const char* id, Vector4f* color, const ColorSelectorButtonData& colorSelectorData) {
    auto& gui = main.g.gui;
    color_button(gui, id, color, {
        .isSelected = colorPickerPtr == color,
        .onClick = [&, colorSelectorData, color] {
            if(colorSelectorData.onSelectorButtonClick) colorSelectorData.onSelectorButtonClick();
            color_selector(color, {
                .onChange = colorSelectorData.onChange,
                .onSelect = colorSelectorData.onSelect,
                .onDeselect = colorSelectorData.onDeselect
            });
        }
    });
}

void CanvasEditorScreen::create_new_tab(const CustomEvents::OpenInfiniPaintFileEvent& openFile) {
    if(!mobileUI) {
        std::shared_ptr<World> newWorld = World::create_world_ptr_from_event(main, openFile);
        if(newWorld) {
            worlds.emplace_back(newWorld);
            switch_to_tab(worlds.size() - 1);
            main.g.gui.set_to_layout();
        }
    }
}

void CanvasEditorScreen::set_tab_to_close(World* world) {
    tabsToClose.emplace(world);
}

void CanvasEditorScreen::input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile) {
    create_new_tab(openFile);
}

void CanvasEditorScreen::switch_to_tab(size_t wIndex) {
    world = worlds[wIndex];
}

void CanvasEditorScreen::set_settings_menu_popup_mode_with_color_picker_data(SettingsMenuPopup newSettingsMode, Vector4f* newColorPickerPtr, const ColorSelectorData& initColorPickerData) {
    settingsMenuPopup = newSettingsMode;
    reset_color_picker_popup_data();
    set_color_picker_data(newColorPickerPtr, initColorPickerData);
}

void CanvasEditorScreen::set_color_picker_data(Vector4f* newColorPickerPtr, const ColorSelectorData& initColorPickerData) {
    colorPickerPtr = newColorPickerPtr;
    colorPickerData = initColorPickerData;
}

SCollision::AABB<float> CanvasEditorScreen::canvas_safe_area() {
    if(main.hideInterface)
        return main.window.safeArea;
    else
        return SCollision::AABB<float>{{main.window.safeArea.min.x(), main.window.safeArea.min.y() + 40.0f * main.g.final_gui_scale()},
                                       {main.window.safeArea.max.x(), main.window.safeArea.max.y() - 30.0f * main.g.final_gui_scale()}};
}

size_t CanvasEditorScreen::open_world_count() {
    return worlds.size();
}

CanvasEditorScreen::~CanvasEditorScreen() {
#ifndef __ANDROID__
    if(mobileUI)
        mobile_save_file();
#endif
}
