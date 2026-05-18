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
#include "../../../DrawData.hpp"
#include "../../../CanvasComponents/CanvasComponentContainer.hpp"
#include "../../../Screens/PhoneDrawingProgramScreen.hpp"

class DrawingProgram;
class EditTool;

class DrawingProgramEditToolBase {
    public:
        DrawingProgramEditToolBase(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp);
        virtual void edit_start(EditTool& editTool, std::any& prevData) = 0;
        virtual void commit_edit_updates(std::any& prevData) = 0;
        virtual void edit_update() = 0;
        virtual void edit_gui(Toolbar& t) = 0;
        virtual void gui_phone_toolbox(PhoneDrawingProgramScreen& t) = 0;
        virtual void right_click_popup_gui(Toolbar& t, Vector2f popupPos);
        virtual Vector4f* color_picker_color(Vector4f* oldColor);
        virtual bool phone_gui_tool_specific_bottom_toolbar_exists();
        virtual void phone_gui_tool_specific_bottom_toolbar(PhoneDrawingProgramScreen& t);
        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste);
        virtual void input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput);
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key);
        virtual void input_text_callback(const InputManager::TextCallbackArgs& text);
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key);
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button, bool isDraggingPoint);
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool isDraggingPoint);
        virtual std::optional<InputManager::TextBoxStartInfo> get_text_box_start_info();
        virtual ~DrawingProgramEditToolBase(); 

        bool commitUpdate = false;
    protected:
        DrawingProgram& drawP;
        CanvasComponentContainer::ObjInfo* comp;
};
