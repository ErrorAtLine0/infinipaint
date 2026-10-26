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

#pragma once
#include "../Screen.hpp"
#include "../../GUIStuff/Elements/ScrollArea.hpp"

class NonNativeFilePickerGUI {
    public:
        void open_file_selector_non_native(MainProgram& main, const std::string& initFilePickerName, const std::vector<Screen::ExtensionFilter>& initExtensionFilters, Screen::OpenFileSelectorCallback initPostSelectionFunc, const std::string& initFileName, bool initIsSaving);
        void layout(MainProgram& main);
        bool is_open();
    private:
        bool isOpen = false;
        void file_picker_gui_done(MainProgram& main);
        void file_picker_gui_refresh_entries(MainProgram& main);
        std::string filePickerWindowName;
        std::vector<Screen::ExtensionFilter> extensionFiltersComplete;
        std::vector<std::string> extensionFilters;
        std::vector<std::filesystem::path> entries;
        std::filesystem::path currentSelectedPath;
        std::string fileName = "";
        bool isSaving = false;
        size_t extensionSelected;
        GUIStuff::ScrollArea* entriesScrollArea = nullptr;
        Screen::OpenFileSelectorCallback postSelectionFunc;
};
