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
#include <functional>
#include <clay.h>
#include <memory>
#include <string>

class CanvasEditorScreen;
class World;

class CanvasEditorCenterPopupGUI {
    public:
        CanvasEditorCenterPopupGUI(CanvasEditorScreen& initScreen);
        void layout();
        void add_world_to_close_popup_data(const std::shared_ptr<World>& w);
        void set_download_file_popup();
        bool app_close_requested();
    private:
        void close_popup();
        void web_version_welcome();
        void download_file_popup();
        void center_obstructing_window_gui(const char* id, Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& innerContent);

        std::string downloadNameSet;
        bool downloadFilePopup = false;

        struct ClosePopupData {
            struct CloseWorldData {
                std::weak_ptr<World> w;
                bool setToSave = true;
            };
            std::vector<CloseWorldData> worldsToClose; // Using a vector to ensure that the worlds are in proper order
            bool closeAppWhenDone = false;
        } closePopupData;

        CanvasEditorScreen& s;
};
