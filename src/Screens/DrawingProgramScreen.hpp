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
#include "Screen.hpp"

class DrawingProgramScreen : public Screen {
    public:
        DrawingProgramScreen(MainProgram& m);
        virtual void update() override;
        virtual void draw(SkCanvas* canvas) override;
        virtual void input_add_file_to_canvas_callback(const CustomEvents::AddFileToCanvasEvent& addFile) override;
        virtual void input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile) override;
        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste) override;
        virtual void input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) override;
        virtual void input_drop_file_callback(const InputManager::DropCallbackArgs& drop) override;
        virtual void input_drop_text_callback(const InputManager::DropCallbackArgs& drop) override;
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_text_callback(const InputManager::TextCallbackArgs& text) override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        virtual void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) override;
        virtual void input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button) override;
        virtual void input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch) override;
        virtual void input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion) override;
        virtual void input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) override;
        virtual void input_multi_finger_touch_callback(const InputManager::MultiFingerTouchCallbackArgs& touch) override;
        virtual void input_multi_finger_motion_callback(const InputManager::MultiFingerMotionCallbackArgs& motion) override;
        virtual void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) override;
        virtual void input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) override;
        virtual void input_window_resize_callback(const InputManager::WindowResizeCallbackArgs& w) override;
        virtual void input_window_scale_callback(const InputManager::WindowScaleCallbackArgs& w) override;
        virtual std::optional<InputManager::TextBoxStartInfo> get_text_box_start_info() override;
};
