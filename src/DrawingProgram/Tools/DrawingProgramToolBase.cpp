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

#include "DrawingProgramToolBase.hpp"

#include "BrushTool.hpp"
#include "EraserTool.hpp"
#include "FillTool.hpp"
#include "PanCanvasTool.hpp"
#include "RectSelectTool.hpp"
#include "LassoSelectTool.hpp"
#include "RectDrawTool.hpp"
#include "EllipseDrawTool.hpp"
#include "TextBoxTool.hpp"
#include "EyeDropperTool.hpp"
#include "ScreenshotTool.hpp"
#include "EditTool.hpp"
#include "GridModifyTool.hpp"
#include "ZoomCanvasTool.hpp"
#include "LineDrawTool.hpp"

DrawingProgramToolBase::DrawingProgramToolBase(DrawingProgram& initDrawP):
    drawP(initDrawP)
{}

DrawingProgramToolBase::~DrawingProgramToolBase() {}

std::unique_ptr<DrawingProgramToolBase> DrawingProgramToolBase::allocate_tool_type(DrawingProgram& drawP, DrawingProgramToolType t) {
    switch(t) {
        case DrawingProgramToolType::BRUSH:
            return std::make_unique<BrushTool>(drawP);
        case DrawingProgramToolType::ERASER:
            return std::make_unique<EraserTool>(drawP);
        case DrawingProgramToolType::FILL:
            return std::make_unique<FillTool>(drawP);
        case DrawingProgramToolType::LASSOSELECT:
            return std::make_unique<LassoSelectTool>(drawP);
        case DrawingProgramToolType::RECTSELECT:
            return std::make_unique<RectSelectTool>(drawP);
        case DrawingProgramToolType::RECTANGLE:
            return std::make_unique<RectDrawTool>(drawP);
        case DrawingProgramToolType::ELLIPSE:
            return std::make_unique<EllipseDrawTool>(drawP);
        case DrawingProgramToolType::TEXTBOX:
            return std::make_unique<TextBoxTool>(drawP);
        case DrawingProgramToolType::EYEDROPPER:
            return std::make_unique<EyeDropperTool>(drawP);
        case DrawingProgramToolType::SCREENSHOT:
            return std::make_unique<ScreenshotTool>(drawP);
        case DrawingProgramToolType::GRIDMODIFY:
            return std::make_unique<GridModifyTool>(drawP);
        case DrawingProgramToolType::EDIT:
            return std::make_unique<EditTool>(drawP);
        case DrawingProgramToolType::ZOOM:
            return std::make_unique<ZoomCanvasTool>(drawP);
        case DrawingProgramToolType::PAN:
            return std::make_unique<PanCanvasTool>(drawP);
        case DrawingProgramToolType::LINE:
            return std::make_unique<LineDrawTool>(drawP);
    }
    return nullptr;
}

Vector4f* DrawingProgramToolBase::color_picker_color(Vector4f* oldColor) { return nullptr; }
void DrawingProgramToolBase::input_paste_callback(const CustomEvents::PasteEvent& paste) {}
void DrawingProgramToolBase::input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) {}
void DrawingProgramToolBase::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {}
void DrawingProgramToolBase::input_text_callback(const InputManager::TextCallbackArgs& text) {}
void DrawingProgramToolBase::input_key_callback(const InputManager::KeyCallbackArgs& key) {}
void DrawingProgramToolBase::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {}
void DrawingProgramToolBase::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {}
void DrawingProgramToolBase::input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button) {}
void DrawingProgramToolBase::input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch) {}
void DrawingProgramToolBase::input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion) {}
void DrawingProgramToolBase::input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) {}
bool DrawingProgramToolBase::phone_gui_tool_specific_bottom_toolbar_exists() { return false; }
void DrawingProgramToolBase::phone_gui_tool_specific_bottom_toolbar(PhoneDrawingProgramScreen& t) {}
std::optional<InputManager::TextBoxStartInfo> DrawingProgramToolBase::get_text_box_start_info() { return std::nullopt; }
