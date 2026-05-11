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

#include "DrawingProgramEditToolBase.hpp"
#include "../../../MainProgram.hpp"

DrawingProgramEditToolBase::DrawingProgramEditToolBase(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp):
    drawP(initDrawP),
    comp(initComp)
{}

void DrawingProgramEditToolBase::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    t.paint_popup(popupPos);
}

Vector4f* DrawingProgramEditToolBase::color_picker_color(Vector4f* oldColor) { return nullptr; }
void DrawingProgramEditToolBase::input_paste_callback(const CustomEvents::PasteEvent& paste) {}
void DrawingProgramEditToolBase::input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) {}
void DrawingProgramEditToolBase::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {}
void DrawingProgramEditToolBase::input_text_callback(const InputManager::TextCallbackArgs& text) {}
void DrawingProgramEditToolBase::input_key_callback(const InputManager::KeyCallbackArgs& key) {}
void DrawingProgramEditToolBase::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button, bool isDraggingPoint) {}
void DrawingProgramEditToolBase::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool isDraggingPoint) {}
bool DrawingProgramEditToolBase::phone_gui_tool_specific_bottom_toolbar_exists() { return false; }
void DrawingProgramEditToolBase::phone_gui_tool_specific_bottom_toolbar(PhoneDrawingProgramScreen& t) {}
std::optional<InputManager::TextBoxStartInfo> DrawingProgramEditToolBase::get_text_box_start_info() { return std::nullopt; }

DrawingProgramEditToolBase::~DrawingProgramEditToolBase() {}
