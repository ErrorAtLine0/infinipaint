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

void DrawingProgramScreen::draw_rotate_position_popup(SkCanvas* canvas, float yPos) {
    auto& rotationTime = main.world->drawData.cam.lastRotationTime;
    rotationTime.update_time_since();
    if(rotationTime < World::ROTATE_POPUP_DISPLAY_TIME) {
        float a = 1.0f - lerp_time<float>(rotationTime, World::ROTATE_POPUP_DISPLAY_TIME, World::ROTATE_POPUP_FADE_START_TIME);
        std::stringstream rotateStrStrm;
        rotateStrStrm << std::fixed << std::setprecision(2) << radians_to_degrees(main.world->drawData.cam.c.rotation) << "°";
        std::string rotateStr = rotateStrStrm.str();
        float coordFontSize = main.g.final_gui_scale() * main.g.gui.io.fontSize;
        SkFont f = main.g.gui.io.get_font(coordFontSize);
        SkFontMetrics metrics;
        f.getMetrics(&metrics);
        float textPixelLength = f.measureText(rotateStr.c_str(), rotateStr.length(), SkTextEncoding::kUTF8, nullptr);
        float fontHeight = (-metrics.fAscent + metrics.fDescent);
        SkPaint textBackgroundPaint;
        textBackgroundPaint.setColor4f(color_mul_alpha(main.g.gui.io.theme->backColor1, 0.8f * a));
        float ROTATE_POPUP_Y_POS = yPos * main.g.final_gui_scale();
        float ROTATE_POPUP_PADDING = 5.0f * main.g.final_gui_scale();
        SkRect textBackgroundRect = SkRect::MakeLTRB(main.window.size.x() * 0.5f - textPixelLength * 0.5f - ROTATE_POPUP_PADDING, ROTATE_POPUP_Y_POS, main.window.size.x() * 0.5f + textPixelLength * 0.5f + ROTATE_POPUP_PADDING, ROTATE_POPUP_Y_POS + fontHeight + ROTATE_POPUP_PADDING);
        SkPaint textPaint;
        if(main.world->drawData.cam.c.rotation == 0.0)
            textPaint.setColor4f(color_mul_alpha(main.g.gui.io.theme->fillColor1, 1.0f * a));
        else
            textPaint.setColor4f(color_mul_alpha(main.g.gui.io.theme->frontColor1, 1.0f * a));
        canvas->drawRect(textBackgroundRect, textBackgroundPaint);
        canvas->drawSimpleText(rotateStr.c_str(), rotateStr.length(), SkTextEncoding::kUTF8, main.window.size.x() * 0.5f - textPixelLength * 0.5f, ROTATE_POPUP_Y_POS + fontHeight, f, textPaint);
    }
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

void DrawingProgramScreen::input_finger_touch_callback(const FingerInput::TouchCallbackArgs& touch) {
    main.world->input_finger_touch_callback(touch);
}

void DrawingProgramScreen::input_window_resize_callback(const InputManager::WindowResizeCallbackArgs& w) {
}

void DrawingProgramScreen::input_window_scale_callback(const InputManager::WindowScaleCallbackArgs& w) {
}

std::optional<InputManager::TextBoxStartInfo> DrawingProgramScreen::get_text_box_start_info() {
    return main.world->get_text_box_start_info();
}
