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

#include "DrawingProgramScreen.hpp"
#include "../MainProgram.hpp"

DrawingProgramScreen::DrawingProgramScreen(MainProgram& m):
    Screen(m)
{}

void DrawingProgramScreen::update() {
    main.world->focus_update();
}

void DrawingProgramScreen::draw(SkCanvas* canvas) {
    main.draw_world(canvas, main.world, main.world->drawData);
}

void DrawingProgramScreen::input_add_file_to_canvas_callback(const CustomEvents::AddFileToCanvasEvent& addFile) {
    main.world->input_add_file_to_canvas_callback(addFile);
}

void DrawingProgramScreen::input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile) {
    main.create_new_tab(openFile);
}

void DrawingProgramScreen::input_paste_callback(const CustomEvents::PasteEvent& paste) {
    main.world->input_paste_callback(paste);
}

void DrawingProgramScreen::input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) {
    main.world->input_android_text_box_input_callback(textboxInput);
}

void DrawingProgramScreen::input_drop_file_callback(const InputManager::DropCallbackArgs& drop) {
    if(std::filesystem::is_regular_file(drop.data)) {
        std::filesystem::path droppedFilePath(drop.data);
        if(droppedFilePath.has_extension() && droppedFilePath.extension().string() == std::string("." + World::FILE_EXTENSION)) {
            CustomEvents::emit_event<CustomEvents::OpenInfiniPaintFileEvent>({
                .isClient = false,
                .filePathSource = droppedFilePath
            });
            return;
        }
    }
    main.world->input_drop_file_callback(drop);
}

void DrawingProgramScreen::input_drop_text_callback(const InputManager::DropCallbackArgs& drop) {
    main.world->input_drop_text_callback(drop);
}

void DrawingProgramScreen::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    main.world->input_key_callback(key);
}

void DrawingProgramScreen::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {
    main.world->input_text_key_callback(key);
}

void DrawingProgramScreen::input_text_callback(const InputManager::TextCallbackArgs& text) {
    main.world->input_text_callback(text);
}

void DrawingProgramScreen::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    main.world->input_mouse_button_callback(button);
}

void DrawingProgramScreen::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    main.world->input_mouse_motion_callback(motion);
}

void DrawingProgramScreen::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) {
    main.world->input_mouse_wheel_callback(wheel);
}

void DrawingProgramScreen::input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button) {
    main.world->input_pen_button_callback(button);
}

void DrawingProgramScreen::input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch) {
    main.world->input_pen_touch_callback(touch);
}

void DrawingProgramScreen::input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion) {
    main.world->input_pen_motion_callback(motion);
}

void DrawingProgramScreen::input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) {
    main.world->input_pen_axis_callback(axis);
}

void DrawingProgramScreen::input_multi_finger_touch_callback(const InputManager::MultiFingerTouchCallbackArgs& touch) {
    main.world->input_multi_finger_touch_callback(touch);
}

void DrawingProgramScreen::input_multi_finger_motion_callback(const InputManager::MultiFingerMotionCallbackArgs& motion) {
    main.world->input_multi_finger_motion_callback(motion);
}

void DrawingProgramScreen::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
}

void DrawingProgramScreen::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {
}

void DrawingProgramScreen::input_window_resize_callback(const InputManager::WindowResizeCallbackArgs& w) {
}

void DrawingProgramScreen::input_window_scale_callback(const InputManager::WindowScaleCallbackArgs& w) {
}

std::optional<InputManager::TextBoxStartInfo> DrawingProgramScreen::get_text_box_start_info() {
    return main.world->get_text_box_start_info();
}
