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
#include <vector>
#include <string>
#include <optional>
#include <functional>
#include <Helpers/Hashes.hpp>

class MainProgram;
namespace GUIStuff {
    class GUIManager;
}

class ProgramSettingsGUI {
    public:
        void top_tabs_layout(MainProgram& main);
        void side_tabs_layout(MainProgram& main);
        void inner_layout(MainProgram& main, float maxWidth);
        bool input_keybind_callback(MainProgram& main, const Vector2ui32& newKey);
        void on_settings_open();
    private:
        void tabs_layout(MainProgram& main, bool wideButtons);
        void reload_theme_list(MainProgram& main);
        void text_button_wide(GUIStuff::GUIManager& gui, const char* id, const char* str, const std::function<void()>& onClick);

        enum class SettingsMenu {
            GENERAL,
            GRAPHICS,
            TABLET,
            TOUCH,
            THEME,
            KEYBINDS,
            DEBUG
        } settingsMenu = SettingsMenu::GENERAL;
        struct ThemeData {
            std::vector<std::string> themeDirList;
            std::optional<size_t> selectedThemeIndex;
            bool openedSaveAsMenu = false;
        } themeData;

        std::optional<unsigned> keybindWaiting;
};
