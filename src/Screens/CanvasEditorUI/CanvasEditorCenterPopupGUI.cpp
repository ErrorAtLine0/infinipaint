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

#include "CanvasEditorCenterPopupGUI.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../CanvasEditorScreen.hpp"
#include "../../MainProgram.hpp"
#include "../../GUIStuff/Elements/LayoutElement.hpp"
#include "../../GUIStuff/Elements/ScrollArea.hpp"
#include "../../GUIStuff/Elements/TextParagraph.hpp"
#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/CheckBoxHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/TextBoxHelpers.hpp"

using namespace GUIStuff;
using namespace ElementHelpers;

CanvasEditorCenterPopupGUI::CanvasEditorCenterPopupGUI(CanvasEditorScreen& initScreen):
    s(initScreen)
{}

void CanvasEditorCenterPopupGUI::layout() {
    std::erase_if(closePopupData.worldsToClose, [](auto& wPair) {
        return wPair.w.expired();
    });

    if(!closePopupData.worldsToClose.empty())
        close_popup();
    else if(downloadFilePopup)
        download_file_popup();
    else if(!s.mobileUI && s.main.conf.viewWebVersionWelcome)
        web_version_welcome();
}

void CanvasEditorCenterPopupGUI::set_download_file_popup() {
    if(!downloadFilePopup) {
        downloadFilePopup = true;
        s.main.g.gui.set_to_layout();
    }
}

void CanvasEditorCenterPopupGUI::download_file_popup() {
    auto& main = s.main;
    auto& gui = main.g.gui;
    center_obstructing_window_gui("download file popup gui", CLAY_SIZING_GROW(0, 500), CLAY_SIZING_FIT(0, gui.io.safeWindowRect.height()), [&] {
        input_text_field(gui, "file name", "File Name", &downloadNameSet);
        left_to_right_line_layout(gui, [&]() {
            text_button(gui, "save", "Save", {
                .wide = true,
                .onClick = [&] {
                    if(!downloadNameSet.empty()) {
                        s.world->save_to_file(downloadNameSet);
                        downloadFilePopup = false;
                    }
                }
            });
            text_button(gui, "cancel", "Cancel", {
                .wide = true,
                .onClick = [&] {
                    downloadFilePopup = false;
                }
            });
        });
    });
}

void CanvasEditorCenterPopupGUI::close_popup() {
    auto& main = s.main;
    auto& gui = main.g.gui;
    auto& io = gui.io;

    center_obstructing_window_gui("Close program popup GUI", CLAY_SIZING_GROW(0, 500), CLAY_SIZING_FIT(0, std::min(600.0f, gui.io.safeWindowRect.height())), [&] {
        text_label(gui, "Files may contain unsaved changes");
        gui.clipping_element<ScrollArea>("close file popup gui scroll area", ScrollArea::Options{
            .scrollVertical = true,
            .clipVertical = true,
            .scrollbarY = ScrollArea::ScrollbarType::NORMAL,
            .xAlign = CLAY_ALIGN_X_CENTER,
            .innerContent = [&](const ScrollArea::InnerContentParameters& innerDim) {
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
                                .sizing = {.width = CLAY_SIZING_FIXED(innerDim.containerDimensions.x() - io.theme->padding1), .height = CLAY_SIZING_FIT(0) },
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
                                gui.element<LayoutElement>("settings popup close button layout", [&] (LayoutElement* l, const Clay_ElementId& lId) {
                                    CLAY(lId, {
                                        .layout = {
                                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                                            .childGap = 0,
                                            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                                        }
                                    }) {
                                        if(l->get_bb().has_value()) {
                                            RichText::TextData fileNameText;
                                            fileNameText.paragraphs.emplace_back(wLock->name);
                                            s.main.g.gui.element<TextParagraph>("file name", TextParagraph::Data{
                                                .text = fileNameText,
                                                .maxGrowX = l->get_bb().value().width(),
                                                .maxGrowY = 0.0f,
                                                .ellipsis = true
                                            });

                                            RichText::TextData filePathText;
                                            filePathText.paragraphs.emplace_back(wLock->filePath.empty() ? "Autosave in " + main.documentsPath.string() : wLock->filePath.string());
                                            filePathText.tStyleMods.emplace_back();
                                            filePathText.tStyleMods.back().pos = {0, 0};
                                            filePathText.tStyleMods.back().mods[RichText::TextStyleModifier::ModifierType::COLOR] = std::make_shared<RichText::ColorTextStyleModifier>(convert_vec4<Vector4f>(gui.io.theme->frontColor2));
                                            s.main.g.gui.element<TextParagraph>("file path", TextParagraph::Data{
                                                .text = filePathText,
                                                .maxGrowX = l->get_bb().value().width(),
                                                .maxGrowY = 0.0f,
                                                .ellipsis = false
                                            });
                                        }
                                    }
                                });
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
                    s.set_tab_to_close(wLock.get());
                }
                closePopupData.worldsToClose.clear();
                if(closePopupData.closeAppWhenDone)
                    main.setToQuit = true;
            }
        });
        text_button(gui, "Discard All", "Discard All", {
            .wide = true,
            .onClick = [&] {
                for(auto& [w, setToSave] : closePopupData.worldsToClose) {
                    auto wLock = w.lock();
                    if(wLock)
                        s.set_tab_to_close(wLock.get());
                }
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

void CanvasEditorCenterPopupGUI::web_version_welcome() {
    auto& main = s.main;
    auto& gui = main.g.gui;
    center_obstructing_window_gui("web version welcome gui", CLAY_SIZING_GROW(0, 700), CLAY_SIZING_FIT(0, gui.io.safeWindowRect.height()), [&] {
        gui.clipping_element<ScrollArea>("close file popup gui scroll area", ScrollArea::Options{
            .scrollVertical = true,
            .clipVertical = true,
            .scrollbarY = ScrollArea::ScrollbarType::NORMAL,
            .xAlign = CLAY_ALIGN_X_CENTER,
            .innerContent = [&](const ScrollArea::InnerContentParameters& innerDim) {
                text_label_centered(gui, "Welcome to the web version of InfiniPaint!");
                text_label(gui, 
R"(This version contains more known issues than the native version of the app. This includes:
- Rare crashes
- If this browser tab is unfocused, or the window is minimized, any InfiniPaint tabs connected online (whether host or client) will be disconnected
- 4GB memory limit. Might be a problem if you're uploading many files/images
- Not multithreaded
- Can't access local fonts

If you like this app, consider downloading the native version for your system)");
            }
        });
        text_button(gui, "got it", "Got It", {
            .wide = true,
            .onClick = [&] {
                main.conf.viewWebVersionWelcome = false;
            }
        });
    });
}

void CanvasEditorCenterPopupGUI::add_world_to_close_popup_data(const std::shared_ptr<World>& w) {
    if(s.mobileUI)
        return;

    auto it = std::find_if(closePopupData.worldsToClose.begin(), closePopupData.worldsToClose.end(), [&](auto& wStruct) {
        return wStruct.w.lock() == w;
    });
    if(it == closePopupData.worldsToClose.end())
        closePopupData.worldsToClose.emplace_back(w, true);
    s.main.g.gui.set_to_layout();
}

bool CanvasEditorCenterPopupGUI::app_close_requested() {
    if(s.mobileUI)
        return true;
    for(auto& w : s.worlds) {
        if(w->should_ask_before_closing())
            add_world_to_close_popup_data(w);
    }
    closePopupData.closeAppWhenDone = true;
    s.main.g.gui.set_to_layout();
    return closePopupData.worldsToClose.empty();
}

void CanvasEditorCenterPopupGUI::center_obstructing_window_gui(const char* id, Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& innerContent) {
    auto& main = s.main;
    auto& gui = main.g.gui;
    auto& io = gui.io;

    gui.set_z_index(1000, [&] {
        gui.element<LayoutElement>(id, [&] (LayoutElement*, const Clay_ElementId& id) {
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
                .floating = {.zIndex = gui.get_z_index(), .attachPoints = {.element = CLAY_ATTACH_POINT_CENTER_CENTER, .parent = CLAY_ATTACH_POINT_CENTER_CENTER}, .attachTo = CLAY_ATTACH_TO_PARENT}
            }) {
                innerContent();
            }
        });
    });
}
