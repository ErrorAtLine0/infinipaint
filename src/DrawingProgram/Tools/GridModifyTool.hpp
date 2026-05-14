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
#include "DrawingProgramToolBase.hpp"
#include "../../WorldGrid.hpp"
#include "Helpers/NetworkingObjects/NetObjWeakPtr.hpp"

class DrawingProgram;

class GridModifyTool : public DrawingProgramToolBase {
    public:
        GridModifyTool(DrawingProgram& initDrawP);
        void set_grid(const NetworkingObjects::NetObjWeakPtr<WorldGrid>& newGrid);
        virtual DrawingProgramToolType get_type() override;
        virtual Vector4f* color_picker_color(Vector4f* oldColor) override;
        virtual void gui_phone_toolbox(PhoneDrawingProgramScreen& t) override;
        virtual void gui_toolbox(Toolbar& t) override;
        virtual void right_click_popup_gui(Toolbar& t, Vector2f popupPos) override;
        virtual void tool_update() override;
        virtual void erase_component(CanvasComponentContainer::ObjInfo* erasedComp) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
    private:
        WorldGrid oldGrid;
        NetworkingObjects::NetObjWeakPtr<WorldGrid> grid;
        unsigned selectionMode = 0;
};
