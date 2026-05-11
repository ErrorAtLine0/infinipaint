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

#include "CustomEvents.hpp"
#include "GUIStuff/GUIManager.hpp"

class MainProgram;

class GUIHolder {
    public:
        GUIHolder(MainProgram& m);

        GUIStuff::GUIManager gui;

        void load_icons_at(const std::filesystem::path& pathToLoad);
        void load_default_theme();
        void save_theme(const std::filesystem::path& configPath, const std::string& themeName);
        bool load_theme(const std::filesystem::path& configPath, const std::string& themeName);

        void update();
        void window_update();
        void delete_cache_surface();
        void draw(SkCanvas* canvas, bool skiaAA);

        void input_paste_callback(const CustomEvents::PasteEvent& paste);
        void input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput);
        void input_text_key_callback(const InputManager::KeyCallbackArgs& key);
        void input_text_callback(const InputManager::TextCallbackArgs& text);
        void input_key_callback(const InputManager::KeyCallbackArgs& key);
        void input_text_key_callback();
        void input_text_input_callback();
        void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button);
        void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion);
        void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel);
        void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch);
        void input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion);

        std::optional<InputManager::TextBoxStartInfo> get_text_box_start_info();

        float final_gui_scale();
    private:
        void calculate_final_gui_scale();
        float final_gui_scale_not_fit();

        float finalCalculatedGuiScale;

        MainProgram& main;
};
