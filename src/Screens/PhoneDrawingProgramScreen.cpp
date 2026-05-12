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

#include "PhoneDrawingProgramScreen.hpp"
#include "../MainProgram.hpp"
#include "DrawingProgramScreen.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIStuff/Elements/GridScrollArea.hpp"
#include "../GUIStuff/Elements/DropDown.hpp"
#include "../GUIStuff/Elements/ColorPicker.hpp"
#include "../GUIStuff/Elements/TreeListing.hpp"
#include "../GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "../GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../GUIStuff/ElementHelpers/TextBoxHelpers.hpp"
#include "FileSelectScreen.hpp"
#include <Helpers/Logger.hpp>

using namespace GUIStuff;
using namespace ElementHelpers;

PhoneDrawingProgramScreen::PhoneDrawingProgramScreen(MainProgram& m):
    DrawingProgramScreen(m)
{}

void PhoneDrawingProgramScreen::update() {
    DrawingProgramScreen::update();
}

void PhoneDrawingProgramScreen::gui_layout_run() {
    main_display();
}

void PhoneDrawingProgramScreen::main_display() {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
            .childGap = main.g.gui.io.theme->childGap1,
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
    }) {
        top_toolbar();
        CLAY_AUTO_ID({
            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
        }) {}
        bottom_toolbar();
    }
}

void PhoneDrawingProgramScreen::top_toolbar() {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    gui.element<LayoutElement>("top toolbar", [&](LayoutElement*, const Clay_ElementId& lId) {
        CLAY(lId, {}) {
            window_fill_side_bar(gui, {
                .dir = WindowFillSideBarConfig::Direction::TOP,
                .backgroundColor = io.theme->backColor0,
                .border = {
                    .color = convert_vec4<Clay_Color>(io.theme->frontColor1),
                    .width = {.bottom = 1}
                }
            }, [&] {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                    }
                }) {
                    svg_icon_button(gui, "back exit button", "data/icons/RemixIcon/arrow-left-s-line.svg", {
                        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                        .onClick = [&] {
                            main.world->save_to_file(main.world->filePath);
                            main.set_tab_to_close(main.world.get());
                            main.set_screen([&] (std::unique_ptr<Screen>) { return std::make_unique<FileSelectScreen>(main); });
                        }
                    });
                    top_toolbar_remaining_area();
                }
            });
        }
    });
}

void PhoneDrawingProgramScreen::top_toolbar_remaining_area() {
    auto& gui = main.g.gui;
    std::vector<TopToolbarRemainingAreaButton> listOfTopToolbarButtons = {
        TopToolbarRemainingAreaButton{
            .name = "Undo",
            .svgPath = "data/icons/undo.svg",
            .onClick = [&] {
                main.world->undo_with_checks();
            }
        },
        TopToolbarRemainingAreaButton{
            .name = "Redo",
            .svgPath = "data/icons/redo.svg",
            .onClick = [&] {
                main.world->redo_with_checks();
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
                constexpr float MINIMUM_SPACE_FOR_NAME = 100.0f;
                constexpr float BUTTON_WIDTH = BIG_BUTTON_SIZE;
                float spaceForWorldName = l->get_bb().value().width() - BUTTON_WIDTH;
                if(spaceForWorldName < 4.0f)
                    spaceForWorldName = 4.0f;
                size_t buttonFreeSpace;
                for(buttonFreeSpace = 0; buttonFreeSpace < listOfTopToolbarButtons.size(); buttonFreeSpace++) {
                    if(spaceForWorldName <= MINIMUM_SPACE_FOR_NAME + BUTTON_WIDTH)
                        break;
                    spaceForWorldName -= BUTTON_WIDTH;
                }
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
                gui.new_id("visible buttons", [&] {
                    for(size_t i = 0; i < buttonFreeSpace; i++) {
                        gui.new_id(i, [&] {
                            svg_icon_button(gui, "button", listOfTopToolbarButtons.front().svgPath, {
                                .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                .onClick = listOfTopToolbarButtons.front().onClick
                            });
                            listOfTopToolbarButtons.erase(listOfTopToolbarButtons.begin());
                        });
                    }
                });
                Element* e = svg_icon_button(gui, "Hidden buttons", "data/icons/RemixIcon/more-2-fill.svg", {
                    .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                    .onClick = [&] {
                        topToolbarButtonPopupOpen = !topToolbarButtonPopupOpen;
                    }
                });
                if(topToolbarButtonPopupOpen)
                    top_toolbar_hidden_button_popup(e, listOfTopToolbarButtons);
            }
        }
    });
}

void PhoneDrawingProgramScreen::top_toolbar_hidden_button_popup(GUIStuff::Element* b, std::vector<TopToolbarRemainingAreaButton> l) {
    auto& gui = main.g.gui;

    l.emplace_back(TopToolbarRemainingAreaButton{
        .name = "Add Image",
        .svgPath = "data/icons/RemixIcon/image-add-line.svg",
        .onClick = [&] {
            open_file_selector("Open File", {{"Any File", "*"}}, [&](const std::filesystem::path& p, const auto& e) {
                CustomEvents::emit_event<CustomEvents::AddFileToCanvasEvent>({
                    .type = CustomEvents::AddFileToCanvasEvent::Type::PATH,
                    .filePath = p,
                    .pos = main.window.size.cast<float>() / 2.0f
                });
            });
        }
    });

    l.emplace_back(TopToolbarRemainingAreaButton{
        .name = "Canvas Color",
        .svgPath = "data/icons/RemixIcon/settings-3-line.svg",
        .onClick = [&] {
            settingsMenuPopup = SettingsMenuPopup::SETTINGS;
            colorPickerPtr = &backgroundColorTemporary;
            colorPickerData = {
                .onChange = [&] {
                    main.world->canvasTheme.set_back_color({backgroundColorTemporary.x(), backgroundColorTemporary.y(), backgroundColorTemporary.z()});
                }
            };
        }
    });

    gui.set_z_index(gui.get_z_index() + 1, [&] {
        dropdown_many_element_popup_layout(gui, "top toolbar hidden button popup", {
            .button = b,
            .clickAwayCallback = [&] { topToolbarButtonPopupOpen = false; },
            .entrySize = {150.0f, 30.0f},
            .entryCount = l.size(),
            .entryLayout = [&] (size_t i) {
                auto& listItem = l[i];
                gui.element<SelectableButton>("button", SelectableButton::Data{
                    .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                    .onClick = [&, oC = listItem.onClick] {
                        oC();
                        topToolbarButtonPopupOpen = false;
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

void PhoneDrawingProgramScreen::bottom_toolbar() {
    auto& gui = main.g.gui;
    auto& io = gui.io;
    window_fill_side_bar(gui, {
        .dir = WindowFillSideBarConfig::Direction::BOTTOM,
    }, [&] {
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                .childGap = io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
            }
        }) {
            if(main.world->drawProg.phone_gui_tool_specific_bottom_toolbar_exists())
                main.world->drawProg.phone_gui_tool_specific_bottom_toolbar(*this);
            else
                default_bottom_toolbar();
        }
    });
}

void PhoneDrawingProgramScreen::default_bottom_toolbar() {
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
                    reset_color_picker_popup_data();
                    colorPickerPtr = nullptr;
                    break;
                case SettingsMenuPopup::SETTINGS:
                    if(colorPickerPtr != &backgroundColorTemporary)
                        colorPickerPtr = main.world->drawProg.color_picker_color(colorPickerPtr);
                    if(colorPickerPtr) {
                        if(colorPickerPopupData.screenType == ColorPickerPopupData::ScreenType::NORMAL) {
                            CLAY_AUTO_ID({
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                }
                            }) {}
                        }
                        color_settings_popup(colorPickerPtr, colorPickerData, [&] {
                            colorPickerPtr = nullptr;
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

void PhoneDrawingProgramScreen::bottom_toolbar_gui() {
    GUIManager& gui = main.g.gui;
    auto& drawP = main.world->drawProg;

    auto tool_button = [&](const char* id, const std::string& svgPath, DrawingProgramToolType toolType) {
        svg_icon_button(gui, id, svgPath, {
            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
            .isSelected = drawP.drawTool->get_type() == toolType,
            .onClick = [&, toolType] {
                if(toolType == DrawingProgramToolType::EDIT)
                    settingsMenuPopup = SettingsMenuPopup::SETTINGS;
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

void PhoneDrawingProgramScreen::tool_settings_popup() {
    auto& drawP = main.world->drawProg;
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

void PhoneDrawingProgramScreen::reset_color_picker_popup_data() {
    colorPickerPopupData.screenType = ColorPickerPopupData::ScreenType::NORMAL;
    colorPickerPopupData.newPaletteStr.clear();
    colorPickerPopupData.paletteListSelection.clear();
}

void PhoneDrawingProgramScreen::color_settings_popup(Vector4f* color, const ColorSelectorData& d, std::function<void()> backOnPaletteButton, bool setAlphaToOne, bool setToTransparentCancelButton) {
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
                .onChange = [&, d] {
                    if(d.onChange) d.onChange();
                    gui.set_to_layout();
                },
                .onHold = d.onSelect,
                .onRelease = d.onDeselect
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

void PhoneDrawingProgramScreen::bottom_extra_toolbar_gui() {
    GUIManager& gui = main.g.gui;

    svg_icon_button(gui, "tool settings", "data/icons/RemixIcon/settings-3-line.svg", {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = settingsMenuPopup == SettingsMenuPopup::SETTINGS,
        .onClick = [&] {
            settingsMenuPopup = (settingsMenuPopup == SettingsMenuPopup::SETTINGS) ? SettingsMenuPopup::NONE : SettingsMenuPopup::SETTINGS;
        }
    });

    color_button(gui, "foreground color", &main.toolConfig.globalConf.foregroundColor, {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = settingsMenuPopup == SettingsMenuPopup::COLOR_CHANGE && colorPickerPtr == &main.toolConfig.globalConf.foregroundColor,
        .hasAlpha = true,
        .onClick = [&] {
            if(settingsMenuPopup == SettingsMenuPopup::COLOR_CHANGE && colorPickerPtr == &main.toolConfig.globalConf.foregroundColor) {
                colorPickerPtr = nullptr;
                settingsMenuPopup = SettingsMenuPopup::NONE;
            }
            else {
                colorPickerPtr = &main.toolConfig.globalConf.foregroundColor;
                settingsMenuPopup = SettingsMenuPopup::COLOR_CHANGE;
            }
            colorPickerData = ColorSelectorData();
        }
    });

    color_button(gui, "background color", &main.toolConfig.globalConf.backgroundColor, {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = settingsMenuPopup == SettingsMenuPopup::COLOR_CHANGE && colorPickerPtr == &main.toolConfig.globalConf.backgroundColor,
        .hasAlpha = true,
        .onClick = [&] {
            if(settingsMenuPopup == SettingsMenuPopup::COLOR_CHANGE && colorPickerPtr == &main.toolConfig.globalConf.backgroundColor) {
                colorPickerPtr = nullptr;
                settingsMenuPopup = SettingsMenuPopup::NONE;
            }
            else {
                colorPickerPtr = &main.toolConfig.globalConf.backgroundColor;
                settingsMenuPopup = SettingsMenuPopup::COLOR_CHANGE;
            }
            colorPickerData = ColorSelectorData();
        }
    });
}

void PhoneDrawingProgramScreen::input_global_back_button_callback() {
    main.world->save_to_file(main.world->filePath);
    main.set_tab_to_close(main.world.get());
    main.g.gui.set_to_layout();
    main.set_screen([&] (std::unique_ptr<Screen>) { return std::make_unique<FileSelectScreen>(main); });
}

void PhoneDrawingProgramScreen::input_app_about_to_go_to_background_callback() {
    main.world->save_to_file(main.world->filePath);
}

void PhoneDrawingProgramScreen::color_selector(Vector4f* color, const ColorSelectorData& colorSelectorData) {
    if(colorPickerPtr != color) {
        colorPickerPtr = color;
        colorPickerData = colorSelectorData;
    }
    else
        colorPickerPtr = nullptr;
    main.g.gui.set_to_layout();
}

void PhoneDrawingProgramScreen::color_selector_button(const char* id, Vector4f* color, const ColorSelectorButtonData& colorSelectorData) {
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
