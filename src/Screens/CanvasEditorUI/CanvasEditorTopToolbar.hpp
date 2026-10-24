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
#include <string>
#include <functional>
#include "../../GUIStuff/Elements/Element.hpp"
#include "AboutDisplayGUI.hpp"
#include "ProgramSettingsGUI.hpp"

class CanvasEditorScreen;

class CanvasEditorTopToolbar {
    public:
        CanvasEditorTopToolbar(CanvasEditorScreen& initScreen);
        void layout_top_toolbar();
        void layout_settings_popup();
        bool input_keybind_callback(MainProgram& main, const Vector2ui32& newKey);
    private:
        struct IconMenuButton {
            std::string name;
            std::string svgPath;
            bool isSelected = false;
            std::function<void()> onClick;
        };

        void desktop_main_menu_popup(GUIStuff::Element* b);
        void safety_layout_checks();
        void global_log();
        void remaining_area();
        void mobile_ui_name_fill();
        void desktop_ui_tabs_fill();
        void hidden_button_popup(GUIStuff::Element* b, std::vector<IconMenuButton> l);
        void icon_menu_dropdown(GUIStuff::Element* b, const char* id, int zIndex, const std::vector<IconMenuButton>& l, const std::function<void()>& clickCallback);

        enum class SettingsPopup {
            NONE,
            HIDDEN_BUTTONS,
            BOOKMARKS,
            LAYERS,
            GRIDS,
            HOST,
            LOBBY_INFO,
            PLAYER_LIST,
            MAIN_SETTINGS,
            ABOUT
        } settingsPopup = SettingsPopup::NONE;
        void settings_popup_default_layout(const char* layoutElemId, const std::function<void()>& layoutFunc, bool isRightAttached = true, float maxWidth = 300.0f);
        void settings_popup_close_button_ontop();
        void settings_popup_close_button();

        bool desktopMainMenuPopupOpen = false;

        std::string serverToConnectTo;
        std::string serverLocalID;

        std::filesystem::path fileOldPath;

        ProgramSettingsGUI settingsMenu;
        AboutDisplayGUI aboutMenu;

        CanvasEditorScreen& s;
};
