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

#include "FileSelectScreen.hpp"
#include "../MainProgram.hpp"
#include "../GUIHolder.hpp"
#include "../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIStuff/Elements/GridScrollArea.hpp"
#include "../GUIStuff/Elements/ManyElementScrollArea.hpp"
#include "../GUIStuff/Elements/ImageDisplay.hpp"
#include "../GUIStuff/Elements/LayoutElement.hpp"
#include "../GUIStuff/Elements/SVGIcon.hpp"
#include "../GUIStuff/Elements/TextParagraph.hpp"
#include "../GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "../GUIStuff/ElementHelpers/ColorPickerHelpers.hpp"
#include "../GUIStuff/ElementHelpers/CheckBoxHelpers.hpp"
#include "../GUIStuff/ElementHelpers/NumberSliderHelpers.hpp"
#include "../World.hpp"
#include "Helpers/StringHelpers.hpp"
#include "PhoneDrawingProgramScreen.hpp"
#include <SDL3/SDL_time.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>

#define MAIN_MENU_SIZE 300

using namespace GUIStuff;
using namespace ElementHelpers;

FileSelectScreen::FileSelectScreen(MainProgram& m): Screen(m) {
    savePath = main.conf.configPath / "saves";
    trashPath = main.conf.configPath / "trash";
    infoPath = main.conf.configPath / "fileSelectInfo.json";

    SDL_CreateDirectory(savePath.string().c_str());
    SDL_CreateDirectory(trashPath.string().c_str());

    try {
        nlohmann::json j(nlohmann::json::parse(read_file_to_string(infoPath)));
        j.get_to(saveInfo);
    }
    catch(...) {}

    update_file_list(fileList, savePath, false);

    // Update trash list once to get trash info up to date
    std::vector<FileInfo> trashListTemp;
    update_file_list(trashListTemp, trashPath, true);
}

void FileSelectScreen::gui_layout_run() {
    main_display();
}

void FileSelectScreen::update_file_list(std::vector<FileInfo>& fL, const std::filesystem::path& folderPath, bool trashUpdate) {
    constexpr SDL_Time NS_IN_DAY = SDL_SECONDS_TO_NS(24 * 3600);
    constexpr int64_t DAYS_TO_DELETION = 30;

    SDL_Time currentTime;
    SDL_GetCurrentTime(&currentTime);

    fL.clear();
    int globCount;
    char** filesInPath = SDL_GlobDirectory(folderPath.string().c_str(), "*", 0, &globCount);
    if(filesInPath) {
        for(int i = 0; i < globCount; i++) {
            std::filesystem::path p = filesInPath[i];
            std::string fExt = p.extension().string();
            if(fExt == World::DOT_FILE_EXTENSION) {
                std::filesystem::path fullPath = folderPath / filesInPath[i];
                FileInfo fileInfoToAdd;
                fileInfoToAdd.fileName = p.stem().string();

                if(trashUpdate) {
                    auto it = saveInfo.trash.files.find(fileInfoToAdd.fileName);
                    if(it == saveInfo.trash.files.end()) {
                        saveInfo.trash.files[fileInfoToAdd.fileName].trashTime = currentTime;
                        fileInfoToAdd.lastModifyTime = currentTime;
                    }
                    else
                        fileInfoToAdd.lastModifyTime = it->second.trashTime;

                    int64_t daysSinceMovedToTrash = (currentTime - fileInfoToAdd.lastModifyTime) / NS_IN_DAY;

                    if(daysSinceMovedToTrash >= DAYS_TO_DELETION) {
                        // Remove file and thumbnail
                        std::filesystem::path thumbnailPath = folderPath / (fileInfoToAdd.fileName + ".jpg");
                        Logger::get().log("INFO", "Deleting file in trash: " + fullPath.string() + " with thumbnail: " + thumbnailPath.string());
                        SDL_RemovePath(fullPath.string().c_str());
                        SDL_RemovePath(thumbnailPath.string().c_str());
                        saveInfo.trash.files.erase(it);
                        continue;
                    }
                    else if(daysSinceMovedToTrash == 29)
                        fileInfoToAdd.lastModifyDate = "Today";
                    else if(daysSinceMovedToTrash == 28)
                        fileInfoToAdd.lastModifyDate = "Tomorrow";
                    else
                        fileInfoToAdd.lastModifyDate = std::to_string(DAYS_TO_DELETION - daysSinceMovedToTrash - 1) + " days";
                }
                else {
                    SDL_PathInfo pathInfo;
                    if(SDL_GetPathInfo(fullPath.string().c_str(), &pathInfo)) {
                        SDL_DateTime pathDt;
                        fileInfoToAdd.lastModifyTime = pathInfo.modify_time;
                        if(SDL_TimeToDateTime(pathInfo.modify_time, &pathDt, true)) {
                            fileInfoToAdd.lastModifyDate = sdl_time_to_nice_access_time(pathDt, main.conf.dateFormat, main.conf.timeFormat);
                        }
                    }
                }

                fL.emplace_back(fileInfoToAdd);
            }
        }
    }
    else
        SDL_CreateDirectory(folderPath.string().c_str());

    sort_file_list(fL);
}

void FileSelectScreen::sort_file_list(std::vector<FileInfo>& fL) {
    switch(saveInfo.fileSort) {
        case SaveInfo::FileSort::ALPHABETICAL_ASCENDING:
            std::stable_sort(fL.begin(), fL.end(), [&](const FileInfo& a, const FileInfo& b) {
                return std::lexicographical_compare(a.fileName.begin(), a.fileName.end(), b.fileName.begin(), b.fileName.end());
            });
            break;
        case SaveInfo::FileSort::ALPHABETICAL_DESCENDING:
            std::stable_sort(fL.begin(), fL.end(), [&](const FileInfo& a, const FileInfo& b) {
                return std::lexicographical_compare(b.fileName.begin(), b.fileName.end(), a.fileName.begin(), a.fileName.end());
            });
            break;
        case SaveInfo::FileSort::TIME_ASCENDING:
            std::stable_sort(fL.begin(), fL.end(), [&](const FileInfo& a, const FileInfo& b) {
                return b.lastModifyTime < a.lastModifyTime;
            });
            break;
        case SaveInfo::FileSort::TIME_DESCENDING:
            std::stable_sort(fL.begin(), fL.end(), [&](const FileInfo& a, const FileInfo& b) {
                return a.lastModifyTime < b.lastModifyTime;
            });
            break;
    }
}

void FileSelectScreen::main_display() {
    auto& gui = main.g.gui;
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
    }) {
        mainMenuOpenAnim = gui.float_animation("main menu animation", {
            .duration = 0.1f,
            .easing = BezierEasing(0.36f, 0.0f, 0.64f, 1.0f)
        });
        actionBarOpenAnim = gui.float_animation("action bar animation", {
            .duration = 0.1f,
            .easing = BezierEasing(0.36f, 0.0f, 0.64f, 1.0f)
        });
        main_menu();
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
        }) {
            if(editMode) {
                numberOfSelectedEntries = std::count_if(fileList.begin(), fileList.end(), [] (const FileInfo& f) { return f.selected; });
                if(numberOfSelectedEntries == 0)
                    actionBarOpenAnim->animation_trigger_reverse();
                else
                    actionBarOpenAnim->animation_trigger();
                edit_title_bar();
            }
            else {
                actionBarOpenAnim->animation_trigger_reverse();
                title_bar();
            }
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT
                },
            }) {
                window_gap_side_bar(gui, WindowFillSideBarConfig::Direction::LEFT);
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                        .padding = CLAY_PADDING_ALL(gui.io.theme->padding1),
                        .childGap = gui.io.theme->childGap1,
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }) {
                    switch(selectedMenu) {
                        case SelectedMenu::FILES:
                            file_view();
                            if(!editMode)
                                create_file_button();
                            break;
                        case SelectedMenu::TRASH:
                            file_view();
                            break;
                        case SelectedMenu::CONNECT:
                            connect_view();
                            break;
                        case SelectedMenu::SETTINGS:
                            settings_view();
                            break;
                    }
                }
                window_gap_side_bar(gui, WindowFillSideBarConfig::Direction::RIGHT);
            }
            window_gap_side_bar(gui, WindowFillSideBarConfig::Direction::BOTTOM);
            edit_action_bar();
            menu_black_box();
        }
    }
}

void FileSelectScreen::create_file_button() {
    auto& gui = main.g.gui;
    gui.set_z_index(gui.get_z_index() + 2, [&] {
        gui.element<LayoutElement>("create file button floating area", [&](LayoutElement*, const Clay_ElementId& lId) {
            CLAY(lId, {
                .layout = {.sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0)}},
                .floating = {
                    .offset = {-25, -25},
                    .zIndex = gui.get_z_index(),
                    .attachPoints = {
                        .element = CLAY_ATTACH_POINT_RIGHT_BOTTOM,
                        .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM
                    },
                    .attachTo = CLAY_ATTACH_TO_PARENT
                }
            }) {
                svg_icon_button(gui, "add button", "data/icons/plus.svg", {
                    .drawType = SelectableButton::DrawType::FILLED_INVERSE,
                    .size = 40.0f,
                    .onClick = [&] {
                        CustomEvents::emit_event<CustomEvents::OpenInfiniPaintFileEvent>({
                            .isClient = false,
                            .saveThumbnail = true,
                            .filePathEmptyAutoSaveDir = savePath
                        });
                    }
                });
            }
        });
    });
}

void FileSelectScreen::edit_action_bar_button(const char* id, const std::string& svgPath, const char* text, const std::function<void()>& onClick) {
    auto& gui = main.g.gui;
    CLAY_AUTO_ID({
        .layout = {.sizing = {.width = CLAY_SIZING_FIXED(70), .height = CLAY_SIZING_GROW(0)}}
    }) {
        gui.element<SelectableButton>(id, SelectableButton::Data{
            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
            .onClick = onClick,
            .innerContent = [&](const SelectableButton::InnerContentCallbackParameters&) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0)},
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    }
                }) {
                    CLAY_AUTO_ID({
                        .layout = {.sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)}}
                    }) {
                        gui.element<SVGIcon>("icon", svgPath, false);
                    }
                    text_label(gui, text);
                }
            }
        });
    }
}

void FileSelectScreen::move_selected_files(const std::filesystem::path& fromPath, const std::filesystem::path& toPath, TrashMoveType trashMoveType) {
    SDL_Time currentTime;
    SDL_GetCurrentTime(&currentTime);

    std::vector<std::string> toFolderListNames;
    try {
        toFolderListNames = glob_path_as_string_list(toPath, "*", 0, [&](const auto& p){ return p.stem().string();});
    } catch(...) {
        SDL_CreateDirectory(toPath.string().c_str());
    }

    for(const FileInfo& f : fileList) {
        if(f.selected) {
            std::string newFileName = ensure_string_unique(toFolderListNames, f.fileName);
            toFolderListNames.emplace_back(newFileName);
            std::filesystem::path filePath = fromPath / (f.fileName + World::DOT_FILE_EXTENSION);
            std::filesystem::path thumbnailPath = fromPath / (f.fileName + ".jpg");
            std::filesystem::path newFilePath = toPath / (newFileName + World::DOT_FILE_EXTENSION);
            std::filesystem::path newThumbnailPath = toPath / (newFileName + ".jpg");
            SDL_RenamePath(filePath.string().c_str(), newFilePath.string().c_str());
            SDL_RenamePath(thumbnailPath.string().c_str(), newThumbnailPath.string().c_str());
            switch(trashMoveType) {
                case TrashMoveType::NONE:
                    break;
                case TrashMoveType::MOVE_TO_TRASH:
                    saveInfo.trash.files[newFileName] = TrashInfo::TrashFile{
                        .trashTime = currentTime
                    };
                    break;
                case TrashMoveType::MOVE_OUT_OF_TRASH:
                    saveInfo.trash.files.erase(f.fileName);
                    break;
            }
        }
    }
}

void FileSelectScreen::duplicate_selected_files(const std::filesystem::path& inPath) {
    std::vector<std::string> toFolderListNames;
    try {
        toFolderListNames = glob_path_as_string_list(inPath, "*", 0, [&](const auto& p){ return p.stem().string();});
    } catch(...) {
        SDL_CreateDirectory(inPath.string().c_str());
    }

    for(const FileInfo& f : fileList) {
        if(f.selected) {
            std::string newFileName = ensure_string_unique(toFolderListNames, f.fileName);
            toFolderListNames.emplace_back(newFileName);
            std::filesystem::path filePath = inPath / (f.fileName + World::DOT_FILE_EXTENSION);
            std::filesystem::path thumbnailPath = inPath / (f.fileName + ".jpg");
            std::filesystem::path newFilePath = inPath / (newFileName + World::DOT_FILE_EXTENSION);
            std::filesystem::path newThumbnailPath = inPath / (newFileName + ".jpg");
            SDL_CopyFile(filePath.string().c_str(), newFilePath.string().c_str());
            SDL_CopyFile(thumbnailPath.string().c_str(), newThumbnailPath.string().c_str());
        }
    }
}

void FileSelectScreen::delete_selected_files_in_trash() {
    for(const FileInfo& f : fileList) {
        if(f.selected) {
            std::filesystem::path filePath = trashPath / (f.fileName + World::DOT_FILE_EXTENSION);
            std::filesystem::path thumbnailPath = trashPath / (f.fileName + ".jpg");
            SDL_RemovePath(filePath.string().c_str());
            SDL_RemovePath(thumbnailPath.string().c_str());
            saveInfo.trash.files.erase(f.fileName);
        }
    }
}

void FileSelectScreen::edit_action_bar() {
    auto& gui = main.g.gui;
    if(actionBarOpenAnim->get_val()) {
        gui.set_z_index(gui.get_z_index() + 3, [&] {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .floating = {
                    .zIndex = gui.get_z_index(),
                    .attachPoints = {
                        .element = CLAY_ATTACH_POINT_LEFT_TOP,
                        .parent = CLAY_ATTACH_POINT_LEFT_TOP,
                    },
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                }
            }) {
                CLAY_AUTO_ID({
                    .layout = { .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}},
                }) {}
                gui.element<LayoutElement>("edit action bar bottom fill", [&](LayoutElement*, const Clay_ElementId& lId) {
                    CLAY(lId, {}) {
                        window_fill_side_bar(gui, {
                            .dir = WindowFillSideBarConfig::Direction::BOTTOM,
                            .backgroundColor = gui.io.theme->backColor1
                        }, [&] {
                            gui.element<LayoutElement>("edit action bar", [&](LayoutElement*, const Clay_ElementId& lId) {
                                CLAY(lId, {
                                    .layout = {
                                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50 * actionBarOpenAnim->get_val())},
                                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                    }
                                }) {
                                    if(actionBarOpenAnim->is_at_end()) {
                                        if(selectedMenu == SelectedMenu::FILES) {
                                            edit_action_bar_button("trash", "data/icons/trash.svg", "Trash", [&] {
                                                move_selected_files(savePath, trashPath, TrashMoveType::MOVE_TO_TRASH);
                                                update_file_list(fileList, savePath, false);
                                                editMode = false;
                                            });
                                            edit_action_bar_button("duplicate", "data/icons/RemixIcon/file-copy-line.svg", "Duplicate", [&] {
                                                duplicate_selected_files(savePath);
                                                update_file_list(fileList, savePath, false);
                                                editMode = false;
                                            });
                                        }
                                        else if(selectedMenu == SelectedMenu::TRASH) {
                                            edit_action_bar_button("restore", "data/icons/RemixIcon/refresh-line.svg", "Restore", [&] {
                                                move_selected_files(trashPath, savePath, TrashMoveType::MOVE_OUT_OF_TRASH);
                                                update_file_list(fileList, trashPath, true);
                                                editMode = false;
                                            });
                                            edit_action_bar_button("delete", "data/icons/trash.svg", "Delete", [&] {
                                                delete_selected_files_in_trash();
                                                update_file_list(fileList, trashPath, true);
                                                editMode = false;
                                            });
                                        }
                                    }
                                }
                            });
                        });
                    }
                });
            }
        });
    }
}

void FileSelectScreen::edit_title_bar() {
    auto& gui = main.g.gui;
    gui.element<LayoutElement>("edit top toolbar", [&](LayoutElement*, const Clay_ElementId& lId) {
        CLAY(lId, {}) {
            window_fill_side_bar(gui, {
                .dir = WindowFillSideBarConfig::Direction::TOP,
                .backgroundColor = gui.io.theme->backColor0
            }, [&] {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                        .padding = CLAY_PADDING_ALL(gui.io.theme->padding1),
                        .childGap = gui.io.theme->childGap1,
                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER },
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    }
                }) {
                    svg_icon_button(gui, "close edit mode button", "data/icons/close.svg", {
                        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                        .onClick = [&] {
                            editMode = false;
                        }
                    });
                    mutable_text_label(gui, "select files label", (numberOfSelectedEntries == 0) ? "Select files" : (std::to_string(numberOfSelectedEntries) + " selected"));
                }
            });
        }
    });
}

void FileSelectScreen::title_bar() {
    auto& gui = main.g.gui;
    gui.element<LayoutElement>("top toolbar", [&](LayoutElement*, const Clay_ElementId& lId) {
        CLAY(lId, {}) {
            window_fill_side_bar(gui, {
                .dir = WindowFillSideBarConfig::Direction::TOP,
                .backgroundColor = gui.io.theme->backColor0
            }, [&] {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                        .padding = CLAY_PADDING_ALL(gui.io.theme->padding1),
                        .childGap = gui.io.theme->childGap1,
                        .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT
                    }
                }) {
                    svg_icon_button(gui, "main settings button", "data/icons/menu.svg", {
                        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                        .onClick = [&] {
                            mainMenuOpenAnim->animation_trigger();
                        }
                    });
                    switch(selectedMenu) {
                        case SelectedMenu::FILES:
                            mutable_text_label(gui, "main screen header text files", "Files");
                            break;
                        case SelectedMenu::TRASH:
                            mutable_text_label(gui, "main screen header text trash", "Trash");
                            break;
                        case SelectedMenu::CONNECT:
                            mutable_text_label(gui, "main screen header text connect", "Connect");
                            break;
                        case SelectedMenu::SETTINGS:
                            mutable_text_label(gui, "main screen header text settings", "Settings");
                            break;
                    }
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                        }
                    }) {}
                    if(selectedMenu == SelectedMenu::FILES || selectedMenu == SelectedMenu::TRASH) {
                        SelectableButton* moreButton = svg_icon_button(gui, "more settings button", "data/icons/RemixIcon/more-2-fill.svg", {
                            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                            .onClick = [&] {
                                moreOptionsMenu = MoreOptionsMenu::MAIN;
                            }
                        });
                        if(moreButton->get_bb().has_value() && moreOptionsMenu != MoreOptionsMenu::CLOSED) {
                            size_t elemCount = 0;
                            switch(moreOptionsMenu) {
                                case MoreOptionsMenu::CLOSED:
                                    break;
                                case MoreOptionsMenu::MAIN:
                                    elemCount = 3;
                                    break;
                                case MoreOptionsMenu::VIEW:
                                    elemCount = 4;
                                    break;
                                case MoreOptionsMenu::SORT:
                                    elemCount = 4;
                                    break;
                            }
                            gui.set_z_index(gui.get_z_index() + 10, [&] {
                                dropdown_many_element_popup_layout(gui, "more options popup", {
                                    .button = moreButton,
                                    .clickUpAwayCallback = [&] {
                                        gui.set_post_callback_func_high_priority([&] { // Cancel actions on other buttons
                                            moreOptionsMenu = MoreOptionsMenu::CLOSED;
                                            main.g.gui.set_to_layout();
                                        });
                                    },
                                    .entrySize = {200.0f, 30.0f},
                                    .entryCount = elemCount,
                                    .entryLayout = [&] (size_t i) {
                                        switch(moreOptionsMenu) {
                                            case MoreOptionsMenu::CLOSED:
                                                break;
                                            case MoreOptionsMenu::MAIN: {
                                                switch(i) {
                                                    case 0: {
                                                        text_transparent_option_button("Edit", "Edit", [&] {
                                                            moreOptionsMenu = MoreOptionsMenu::CLOSED;
                                                            start_edit_mode();
                                                        });
                                                        break;
                                                    }
                                                    case 1: {
                                                        text_transparent_option_button("View", "View", [&] {
                                                            moreOptionsMenu = MoreOptionsMenu::VIEW;
                                                        });
                                                        break;
                                                    }
                                                    case 2: {
                                                        text_transparent_option_button("Sort", "Sort", [&] {
                                                            moreOptionsMenu = MoreOptionsMenu::SORT;
                                                        });
                                                        break;
                                                    }
                                                }
                                                break;
                                            }
                                            case MoreOptionsMenu::VIEW: {
                                                switch(i) {
                                                    case 0: {
                                                        text_transparent_option_button("Large Grid", "Large Grid", [&] {
                                                            saveInfo.fileViewType = SaveInfo::FileViewType::LARGE_GRID;
                                                            moreOptionsMenu = MoreOptionsMenu::CLOSED;
                                                        });
                                                        break;
                                                    }
                                                    case 1: {
                                                        text_transparent_option_button("Medium Grid", "Medium Grid", [&] {
                                                            saveInfo.fileViewType = SaveInfo::FileViewType::MEDIUM_GRID;
                                                            moreOptionsMenu = MoreOptionsMenu::CLOSED;
                                                        });
                                                        break;
                                                    }
                                                    case 2: {
                                                        text_transparent_option_button("Small Grid", "Small Grid", [&] {
                                                            saveInfo.fileViewType = SaveInfo::FileViewType::SMALL_GRID;
                                                            moreOptionsMenu = MoreOptionsMenu::CLOSED;
                                                        });
                                                        break;
                                                    }
                                                    case 3: {
                                                        text_transparent_option_button("List", "List", [&] {
                                                            saveInfo.fileViewType = SaveInfo::FileViewType::LIST;
                                                            moreOptionsMenu = MoreOptionsMenu::CLOSED;
                                                        });
                                                        break;
                                                    }
                                                }
                                                break;
                                            }
                                            case MoreOptionsMenu::SORT: {
                                                switch(i) {
                                                    case 0: {
                                                        text_transparent_option_button("Alphabetical Ascending", "Alphabetical ↑", [&] {
                                                            saveInfo.fileSort = SaveInfo::FileSort::ALPHABETICAL_ASCENDING;
                                                            sort_file_list(fileList);
                                                        });
                                                        break;
                                                    }
                                                    case 1: {
                                                        text_transparent_option_button("Alphabetical Descending", "Alphabetical ↓", [&] {
                                                            saveInfo.fileSort = SaveInfo::FileSort::ALPHABETICAL_DESCENDING;
                                                            sort_file_list(fileList);
                                                        });
                                                        break;
                                                    }
                                                    case 2: {
                                                        text_transparent_option_button("Time Ascending", "Date modified ↑", [&] {
                                                            saveInfo.fileSort = SaveInfo::FileSort::TIME_ASCENDING;
                                                            sort_file_list(fileList);
                                                        });
                                                        break;
                                                    }
                                                    case 3: {
                                                        text_transparent_option_button("Time Descending", "Date modified ↓", [&] {
                                                            saveInfo.fileSort = SaveInfo::FileSort::TIME_DESCENDING;
                                                            sort_file_list(fileList);
                                                        });
                                                        break;
                                                    }
                                                }
                                                break;
                                            }
                                        }
                                    }
                                });
                            });
                        }
                    }
                }
            });
        }
    });
}

void FileSelectScreen::text_transparent_option_button(const char* id, const char* text, const std::function<void()>& onClick) {
    auto& gui = main.g.gui;
    text_button(gui, id, text, {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .wide = true,
        .growHeight = true,
        .centered = false,
        .onClick = onClick
    });
}

void FileSelectScreen::icon_text_transparent_option_selected_button(const char* id, const std::string& svgPath, const char* text, bool isSelected, const std::function<void()>& onClick) {
    auto& gui = main.g.gui;
    text_button_with_icon(gui, id, svgPath, text, {
        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
        .isSelected = isSelected,
        .wide = true,
        .centered = false,
        .onClick = onClick
    });
}

void FileSelectScreen::main_menu() {
    if(!mainMenuOpenAnim->is_at_start()) {
        auto& gui = main.g.gui;
        gui.element<LayoutElement>("main menu popup layout element", [&](LayoutElement*, const Clay_ElementId& lId) {
            CLAY(lId, {}) {
                window_fill_side_bar(gui, {
                    .dir = WindowFillSideBarConfig::Direction::LEFT,
                    .backgroundColor = gui.io.theme->backColor0
                }, [&] {
                    gui.element<LayoutElement>("inner main menu element", [&] (LayoutElement*, const Clay_ElementId& lId) {
                        CLAY(lId, {
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_FIXED(200.0f * mainMenuOpenAnim->get_val()), .height = CLAY_SIZING_GROW(0)},
                                .padding = CLAY_PADDING_ALL(gui.io.theme->padding1),
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                        }) {
                            if(mainMenuOpenAnim->is_at_end()) {
                                icon_text_transparent_option_selected_button("Files", "data/icons/folder.svg", "Files", selectedMenu == SelectedMenu::FILES, [&] {
                                    selectedMenu = SelectedMenu::FILES;
                                    update_file_list(fileList, savePath, false);
                                    mainMenuOpenAnim->animation_trigger_reverse();
                                    if(mainViewScrollArea) mainViewScrollArea->reset_scroll();
                                });
                                icon_text_transparent_option_selected_button("Trash", "data/icons/trash.svg", "Trash", selectedMenu == SelectedMenu::TRASH, [&] {
                                    selectedMenu = SelectedMenu::TRASH;
                                    update_file_list(fileList, trashPath, true);
                                    mainMenuOpenAnim->animation_trigger_reverse();
                                    if(mainViewScrollArea) mainViewScrollArea->reset_scroll();
                                });
                                icon_text_transparent_option_selected_button("Connect", "data/icons/network.svg", "Connect", selectedMenu == SelectedMenu::CONNECT, [&] {
                                    selectedMenu = SelectedMenu::CONNECT;
                                    mainMenuOpenAnim->animation_trigger_reverse();
                                    if(mainViewScrollArea) mainViewScrollArea->reset_scroll();
                                });
                                icon_text_transparent_option_selected_button("Settings", "data/icons/RemixIcon/settings-3-line.svg", "Settings", selectedMenu == SelectedMenu::SETTINGS, [&] {
                                    selectedMenu = SelectedMenu::SETTINGS;
                                    mainMenuOpenAnim->animation_trigger_reverse();
                                    if(mainViewScrollArea) mainViewScrollArea->reset_scroll();
                                });
                            }
                        }
                    });
                });
            }
        });
    }
}

void FileSelectScreen::file_view() {
    auto& gui = main.g.gui;
    std::filesystem::path folderPath = (selectedMenu == SelectedMenu::TRASH) ? trashPath : savePath;

    auto fileButton = [&] (size_t i, bool isList, const Vector2f& iconSize) {
        std::filesystem::path filePath = folderPath / (fileList[i].fileName + World::DOT_FILE_EXTENSION);
        gui.element<SelectableButton>("file button", SelectableButton::Data{
            .isSelected = editMode && fileList[i].selected,
            .onClick = [&, filePath, i] {
                if(selectedMenu == SelectedMenu::TRASH) {
                    if(!editMode)
                        start_edit_mode();
                    fileList[i].selected = !fileList[i].selected;
                }
                else {
                    if(editMode)
                        fileList[i].selected = !fileList[i].selected;
                    else {
                        CustomEvents::emit_event<CustomEvents::OpenInfiniPaintFileEvent>({
                            .isClient = false,
                            .saveThumbnail = true,
                            .filePathSource = filePath
                        });
                    }
                }
            },
            .innerContent = [&](const SelectableButton::InnerContentCallbackParameters& c){
                if(isList) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .padding = CLAY_PADDING_ALL(gui.io.theme->padding1),
                            .childGap = 6,
                            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER },
                            .layoutDirection = CLAY_LEFT_TO_RIGHT
                        }
                    }) {
                        CLAY_AUTO_ID({
                            .layout = { .sizing = {.width = CLAY_SIZING_FIXED(iconSize.x()), .height = CLAY_SIZING_FIXED(iconSize.y()) }},
                        }) {
                            gui.element<ImageDisplay>("ico", ImageDisplay::Data{
                                .imgPath = folderPath / (fileList[i].fileName + ".jpg"),
                                .radius = 20
                            });
                        }
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                                .childGap = 6,
                                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER },
                                .layoutDirection = CLAY_TOP_TO_BOTTOM
                            }
                        }) {
                            mutable_text_label(gui, "file name label", fileList[i].fileName);
                            mutable_text_label_light(gui, "last modify date", fileList[i].lastModifyDate);
                        }
                    }
                }
                else {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .padding = CLAY_PADDING_ALL(gui.io.theme->padding1),
                            .childGap = 6,
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        }
                    }) {
                        CLAY_AUTO_ID({
                            .layout = { .sizing = {.width = CLAY_SIZING_FIXED(iconSize.x()), .height = CLAY_SIZING_FIXED(iconSize.y()) }},
                        }) {
                            gui.element<ImageDisplay>("ico", ImageDisplay::Data{
                                .imgPath = folderPath / (fileList[i].fileName + ".jpg"),
                                .radius = 20
                            });
                        }
                        gui.element<LayoutElement>("file name layout elem", [&](LayoutElement* l, const Clay_ElementId& lId) {
                            CLAY(lId, {
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0)},
                                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
                                },
                            }) {
                                if(l->get_bb()) {
                                    RichText::TextData fileNameText;
                                    fileNameText.paragraphs.emplace_back(fileList[i].fileName);
                                    gui.element<TextParagraph>("file name", TextParagraph::Data{
                                        .text = fileNameText,
                                        .maxGrowX = l->get_bb().value().width(),
                                        .maxGrowY = 0.0f
                                    });
                                }
                            }
                        });
                        mutable_text_label_light(gui, "last modify date", fileList[i].lastModifyDate);
                    }
                }
            }
        });
    };

    if(saveInfo.fileViewType == SaveInfo::FileViewType::LIST) {
        mainViewScrollArea = gui.element<ManyElementScrollArea>("File selector list", ManyElementScrollArea::Options{
            .entryHeight = 150.0f,
            .entryCount = fileList.size(),
            .clipHorizontal = true,
            .xElementSize = CLAY_SIZING_GROW(0),
            .elementContent = [&](size_t i) {
                fileButton(i, true, Vector2f{100.0f, 100.0f});
            }
        })->scrollArea;
    }
    else {
        Vector2f entrySize{0.0f, 0.0f};
        Vector2f iconSize{0.0f, 0.0f};
        switch(saveInfo.fileViewType) {
            case SaveInfo::FileViewType::LARGE_GRID:
                entrySize = {170.0f, 210.0f};
                iconSize = {130.0f, 130.0f};
                break;
            case SaveInfo::FileViewType::MEDIUM_GRID:
                entrySize = {140.0f, 180.0f};
                iconSize = {100.0f, 100.0f};
                break;
            case SaveInfo::FileViewType::SMALL_GRID:
                entrySize = {110.0f, 150.0f};
                iconSize = {70.0f, 70.0f};
                break;
            default:
                break;
        }
        mainViewScrollArea = gui.element<GridScrollArea>("File selector grid", GridScrollArea::Options{
            .entryWidth = entrySize.x(),
            .childAlignmentX = CLAY_ALIGN_X_LEFT,
            .entryHeight = entrySize.y(),
            .entryCount = fileList.size(),
            .entryWidthIsMinimum = true,
            .elementContent = [&](size_t i) {
                fileButton(i, false, iconSize);
            }
        })->scrollArea;
    }
}

void FileSelectScreen::connect_view() {
    auto& gui = main.g.gui;

    mainViewScrollArea = gui.element<ScrollArea>("connect scroll area", ScrollArea::Options{
        .scrollVertical = true,
        .clipVertical = true,
        .scrollbarY = ScrollArea::ScrollbarType::NORMAL,
        .innerContent = [&] (auto&) {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT
                },
            }) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0, 300), .height = CLAY_SIZING_GROW(0)},
                        .childGap = gui.io.theme->childGap1,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM
                    },
                }) {
                    input_text_field(gui, "Connect lobby field", "Lobby", &connectLobbyStr);
                    text_button(gui, "Connect button", "Connect", {
                        .wide = true,
                        .onClick = [&] {
                            CustomEvents::emit_event<CustomEvents::OpenInfiniPaintFileEvent>({
                                .isClient = true,
                                .netSource = connectLobbyStr
                            });
                        }
                    });
                    text_button(gui, "Paste and Connect button", "Paste & Connect", {
                        .wide = true,
                        .onClick = [&] {
                            connectOnPaste = true;
                            main.input.call_paste(CustomEvents::PasteEvent::DataType::TEXT, { .allowRichText = false });
                        }
                    });
                }
            }
        }
    });
}

void FileSelectScreen::settings_view() {
    auto& gui = main.g.gui;

    mainViewScrollArea = gui.element<ScrollArea>("settings scroll area", ScrollArea::Options{
        .scrollVertical = true,
        .clipVertical = true,
        .scrollbarY = ScrollArea::ScrollbarType::NORMAL,
        .innerContent = [&] (auto&) {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT
                },
            }) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0, 300), .height = CLAY_SIZING_GROW(0)},
                        .childGap = gui.io.theme->childGap1,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM
                    },
                }) {
                    input_text_field(gui, "display name input", "Display name", &main.conf.displayName);
                    color_picker_button_field(gui, "defaultCanvasBackgroundColor", "Default canvas background color", &main.conf.defaultCanvasBackgroundColor, { .hasAlpha = false });
                    input_scalar_field(gui, "Max GUI Scale", "Max GUI Scale", &main.conf.guiScale, 1.0f, 2.0f, {
                        .decimalPrecision = 1,
                        .onEdit = [&] { main.g.window_update(); }
                    });
                    input_scalar_field(gui, "jump transition time", "Jump transition time", &main.conf.jumpTransitionTime, 0.01f, 1000.0f, {.decimalPrecision = 2});
                    checkbox_boolean_field(gui, "disable touch when pen in proximity", "Disable touch when pen in proximity", &main.conf.tabletOptions.disableTouchWhenPenInProximity);
                    checkbox_boolean_field(gui, "make all tools share same size", "Make all tools share size", &main.toolConfig.globalConf.useGlobalRelativeWidth);
                    slider_scalar_field(gui, "tablet brush minimum size", "Brush relative minimum size", &main.conf.tabletOptions.brushMinimumSize, 0.0f, 1.0f, {.decimalPrecision = 3});
                    slider_scalar_field(gui, "tablet brush pressure smoothing factor", "Brush pressure smoothing factor", &main.conf.tabletOptions.brushPressureSmoothingFactor, 0.0f, 1.0f, {.decimalPrecision = 3});
                    checkbox_boolean_field(gui, "pen pressure width", "Pen pressure affects brush size", &main.conf.tabletOptions.pressureAffectsBrushWidth);
#ifndef __ANDROID__
                    checkbox_boolean_field(gui, "use mobile UI", "Use mobile UI (requires restart)", &main.conf.mobileUI);
#endif
                }
            }
        }
    });
}

void FileSelectScreen::menu_black_box() {
    auto& gui = main.g.gui;
    if(!mainMenuOpenAnim->is_at_start()) {
        gui.set_z_index(gui.get_z_index() + 1, [&] {
            gui.element<LayoutElement>("file list disable fill", [&] (LayoutElement* l, const Clay_ElementId& lId) {
                CLAY(lId, {
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                    },
                    .backgroundColor = {0.0f, 0.0f, 0.0f, 0.6f * mainMenuOpenAnim->get_val()},
                    .floating = {
                        .zIndex = gui.get_z_index(),
                        .attachPoints = {
                            .element = CLAY_ATTACH_POINT_LEFT_TOP,
                            .parent = CLAY_ATTACH_POINT_LEFT_TOP
                        },
                        .attachTo = CLAY_ATTACH_TO_PARENT
                    }
                }) {
                }
            }, LayoutElement::Callbacks{
                .onClick = [&] (LayoutElement* l, const InputManager::MouseButtonCallbackArgs& m) {
                    if(m.down && m.button == InputManager::MouseButton::LEFT && l->mouseHovering) {
                        mainMenuOpenAnim->animation_trigger_reverse();
                        gui.set_to_layout();
                    }
                }
            });
        });
    }
}

void FileSelectScreen::start_edit_mode() {
    for(auto& f : fileList)
        f.selected = false;
    editMode = true;
}

void FileSelectScreen::input_paste_callback(const CustomEvents::PasteEvent& paste) {
    if(connectOnPaste && paste.type == CustomEvents::PasteEvent::DataType::TEXT) {
        CustomEvents::emit_event<CustomEvents::OpenInfiniPaintFileEvent>({
            .isClient = true,
            .netSource = paste.data
        });
    }
}

void FileSelectScreen::input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile) {
    main.create_new_tab(openFile);
    main.set_screen([&] (std::unique_ptr<Screen>) { return std::make_unique<PhoneDrawingProgramScreen>(main); });
}

void FileSelectScreen::input_global_back_button_callback() {
    if(!mainMenuOpenAnim->is_at_start())
        mainMenuOpenAnim->animation_trigger_reverse();
    else if(editMode)
        editMode = false;
    else
        main.setToQuit = true;
    main.g.gui.set_to_layout();
}

void FileSelectScreen::draw(SkCanvas* canvas) {
    canvas->clear(main.g.gui.io.theme->backColor0);
}

void FileSelectScreen::save_files() {
    nlohmann::json j;
    nlohmann::to_json(j, saveInfo);
    std::string saveJson = nlohmann::to_string(j);
    SDL_SaveFile(infoPath.string().c_str(), saveJson.c_str(), saveJson.size());
}

void FileSelectScreen::input_app_about_to_go_to_background_callback() {
    save_files();
}

FileSelectScreen::~FileSelectScreen() {
#ifndef __ANDROID__
    save_files();
#endif
}
