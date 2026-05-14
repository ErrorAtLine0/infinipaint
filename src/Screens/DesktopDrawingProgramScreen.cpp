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

#include "DesktopDrawingProgramScreen.hpp"
#include "../MainProgram.hpp"
#include "DrawingProgramScreen.hpp"

DesktopDrawingProgramScreen::DesktopDrawingProgramScreen(MainProgram& m):
    DrawingProgramScreen(m),
    toolbar(m, *this)
{}

void DesktopDrawingProgramScreen::update() {
    toolbar.update();
    DrawingProgramScreen::update();
}

void DesktopDrawingProgramScreen::gui_layout_run() {
    toolbar.layout_run();
}

bool DesktopDrawingProgramScreen::app_close_requested() {
    return toolbar.app_close_requested();
}

void DesktopDrawingProgramScreen::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    switch(key.key) {
        case InputManager::KEY_NOGUI: {
            if(key.down && !key.repeat) {
                toolbar.drawGui = !toolbar.drawGui;
                main.g.gui.set_to_layout();
            }
            break;
        }
        case InputManager::KEY_SAVE: {
            if(key.down && !key.repeat)
                toolbar.save_func();
            break;
        }
        case InputManager::KEY_SAVE_AS: {
            if(key.down && !key.repeat)
                toolbar.save_as_func();
            break;
        }
        case InputManager::KEY_OPEN_CHAT: {
            if(key.down && !key.repeat)
                toolbar.open_chatbox();
            break;
        }
    }
    DrawingProgramScreen::input_key_callback(key);
}

void DesktopDrawingProgramScreen::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {
    switch(key.key) {
        case InputManager::KEY_GENERIC_ESCAPE: {
            if(key.down && !key.repeat)
                toolbar.close_chatbox();
            break;
        }
    }
    DrawingProgramScreen::input_text_key_callback(key);
}

void DesktopDrawingProgramScreen::open_file_selector(const std::string& filePickerName, const std::vector<ExtensionFilter>& extensionFilters, OpenFileSelectorCallback postSelectionFunc, const std::string& fileName, bool isSaving) {
    if(main.conf.useNativeFilePicker)
        Screen::open_file_selector(filePickerName, extensionFilters, postSelectionFunc, fileName, isSaving);
    else
        toolbar.open_file_selector_non_native(filePickerName, extensionFilters, postSelectionFunc, fileName, isSaving);
}

void DesktopDrawingProgramScreen::on_tab_close() {
    if(main.worlds.empty())
        main.create_new_tab({
            .isClient = false
        });
    else if(main.world)
        main.worldIndex = std::find(main.worlds.begin(), main.worlds.end(), main.world) - main.worlds.begin();
    else
        main.switch_to_tab(0);
}
