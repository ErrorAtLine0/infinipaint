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
#include <include/core/SkCanvas.h>
#include "../../DrawData.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"
#include "../../InputManager.hpp"

class DrawingProgram;
class PhoneDrawingProgramScreen;

#define MINIMUM_DISTANCE_BETWEEN_BOUNDS 10.0f

enum class DrawingProgramToolType : int {
    BRUSH = 0,
    ERASER,
    FILL,
    LASSOSELECT,
    RECTSELECT,
    RECTANGLE,
    ELLIPSE,
    TEXTBOX,
    EYEDROPPER,
    SCREENSHOT,
    GRIDMODIFY,
    EDIT,
    ZOOM,
    PAN,
    LINE
};

class DrawingProgramToolBase {
    public:
        DrawingProgramToolBase(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() = 0;
        virtual void gui_toolbox(Toolbar& t) = 0;
        virtual void gui_phone_toolbox(PhoneDrawingProgramScreen& t) = 0;
        virtual void erase_component(CanvasComponentContainer::ObjInfo* erasedComp) = 0;
        virtual void right_click_popup_gui(Toolbar& t, Vector2f popupPos) = 0;
        virtual void tool_update() = 0;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) = 0;
        virtual void switch_tool(DrawingProgramToolType newTool) = 0;
        virtual bool prevent_undo_or_redo() = 0;
        virtual Vector4f* color_picker_color(Vector4f* oldColor);
        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste);
        virtual void input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput);
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key);
        virtual void input_text_callback(const InputManager::TextCallbackArgs& text);
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key);
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button);
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion);
        virtual void input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button);
        virtual void input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch);
        virtual void input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion);
        virtual void input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis);
        virtual bool phone_gui_tool_specific_bottom_toolbar_exists();
        virtual void phone_gui_tool_specific_bottom_toolbar(PhoneDrawingProgramScreen& t);
        virtual std::optional<InputManager::TextBoxStartInfo> get_text_box_start_info();
        virtual ~DrawingProgramToolBase(); 
        static std::unique_ptr<DrawingProgramToolBase> allocate_tool_type(DrawingProgram& drawP, DrawingProgramToolType t);
    protected:
        DrawingProgram& drawP;
};
