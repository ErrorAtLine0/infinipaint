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

#include "NonNativeFilePickerGUI.hpp"
#include "../../MainProgram.hpp"
#include <Helpers/Logger.hpp>
#include "../../FileHelpers.hpp"
#include "../../GUIStuff/Elements/DropDown.hpp"
#include "../../GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/TextBoxHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "../../GUIStuff/Elements/ManyElementScrollArea.hpp"

using namespace GUIStuff;
using namespace ElementHelpers;

void NonNativeFilePickerGUI::open_file_selector_non_native(MainProgram& main, const std::string& initFilePickerName, const std::vector<Screen::ExtensionFilter>& initExtensionFilters, Screen::OpenFileSelectorCallback initPostSelectionFunc, const std::string& initFileName, bool initIsSaving) {
    isOpen = true;
    extensionFiltersComplete = initExtensionFilters;
    extensionFilters.clear();
    for(auto& [name, exList] : initExtensionFilters)
        extensionFilters.emplace_back(exList);
    extensionSelected = initExtensionFilters.size() - 1;
    filePickerWindowName = initFilePickerName;
    postSelectionFunc = initPostSelectionFunc;
    fileName = "";
    isSaving = initIsSaving;
    entriesScrollArea = nullptr;
    file_picker_gui_refresh_entries(main);
}

bool NonNativeFilePickerGUI::is_open() {
    return isOpen;
}

void NonNativeFilePickerGUI::layout(MainProgram& main) {
    auto& gui = main.g.gui;
    auto& io = gui.io;

    text_label_centered(gui, filePickerWindowName);
    left_to_right_line_layout(gui, [&]() {
        svg_icon_button(gui, "file picker back button", "data/icons/backarrow.svg", {
            .onClick = [&] {
                main.conf.currentSearchPath = main.conf.currentSearchPath.parent_path();
                file_picker_gui_refresh_entries(main);
            }
        });
        input_path_field(gui, "file picker path", "Path", &main.conf.currentSearchPath, {
            .fileTypeRestriction = std::filesystem::file_type::directory,
            .onEdit = [&] {
                file_picker_gui_refresh_entries(main);
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
        entriesScrollArea = gui.element<ManyElementScrollArea>("file picker entries", ManyElementScrollArea::Options{
            .entryHeight = entryHeight,
            .entryCount = entries.size(),
            .clipHorizontal = false,
            .elementContent = [&] (size_t i) {
                const std::filesystem::path& entry = entries[i];
                bool selectedEntry = currentSelectedPath == entry;
                gui.element<LayoutElement>("elem", [&] (LayoutElement*, const Clay_ElementId& lId) {
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
                    .onClick = [&, selectedEntry, entry](LayoutElement* l, const InputManager::MouseButtonCallbackArgs& button) {
                        if(l->mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down) {
                            gui.set_post_callback_func([&, button, selectedEntry, entry] {
                                if(selectedEntry && button.clicks >= 2) {
                                    if(std::filesystem::is_directory(entry)) {
                                        main.conf.currentSearchPath = entry;
                                        file_picker_gui_refresh_entries(main);
                                    }
                                    else if(std::filesystem::is_regular_file(entry))
                                        file_picker_gui_done(main);
                                }
                                else {
                                    currentSelectedPath = entry;
                                    if(std::filesystem::is_regular_file(entry))
                                        fileName = entry.filename().string();
                                }
                            });
                            gui.set_to_layout();
                        }
                    }
                });
            }
        })->scrollArea;
    }
    left_to_right_line_layout(gui, [&]() {
        input_text(gui, "filepicker filename", &fileName);
        gui.element<DropDown<size_t>>("filepicker select type", &extensionSelected, extensionFilters, DropdownOptions{
            .onClick = [&] { file_picker_gui_refresh_entries(main); }
        });
    });
    left_to_right_line_layout(gui, [&]() {
        text_button(gui, "filepicker done", "Done", {
            .wide = true,
            .onClick = [&] { file_picker_gui_done(main); }
        });
        text_button(gui, "filepicker cancel", "Cancel", {
            .wide = true,
            .onClick = [&] { isOpen = false; }
        });
    });
}

void NonNativeFilePickerGUI::file_picker_gui_refresh_entries(MainProgram& main) {
    auto& gui = main.g.gui;

    entries.clear();
    for(;;) {
        try {
            for(const std::filesystem::path& entry : std::filesystem::directory_iterator(main.conf.currentSearchPath))
                entries.emplace_back(entry);
            break;
        }
        catch(const std::exception& e) {
            Logger::get().log(Logger::LogType::INFO, e.what());
            if(main.conf.currentSearchPath == main.homePath) // The home path must exist. If we get errors on the home path, we have a real problem
                throw e;
            main.conf.currentSearchPath = main.homePath;
        }
    }

    std::vector<std::string> extensionList = split_string_by_token(extensionFilters[extensionSelected], ";");
    for(std::string& s : extensionList) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        s.insert(0, ".");
    }

    std::erase_if(entries, [&](const std::filesystem::path& a) {
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

    std::sort(entries.begin(), entries.end(), [&](const std::filesystem::path& a, const std::filesystem::path& b) {
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

    if(entriesScrollArea)
        entriesScrollArea->reset_scroll();

    gui.set_to_layout();
}

void NonNativeFilePickerGUI::file_picker_gui_done(MainProgram& main) {
    if(!fileName.empty()) {
        std::filesystem::path pathToRet = main.conf.currentSearchPath / fileName;
        if(isSaving)
            pathToRet = force_extension_on_path(pathToRet, extensionFiltersComplete[extensionSelected].extensions);
        postSelectionFunc(pathToRet, extensionFiltersComplete[extensionSelected]);
    }
    entriesScrollArea = nullptr;
    isOpen = false;
}
