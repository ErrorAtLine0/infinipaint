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
#include <Helpers/Hashes.hpp>
#include <include/core/SkPath.h>
#include <include/core/SkVertices.h>
#include <Helpers/SCollision.hpp>
#include <include/core/SkCanvas.h>
#include "../../CanvasComponents/CanvasComponentContainer.hpp"
#include <Helpers/NetworkingObjects/NetObjWeakPtr.hpp>
#include "../../CanvasComponents/BrushComponentCode.hpp"

class DrawingProgram;
struct DrawData;
class BrushStrokeCanvasComponent;

class BrushTool : public DrawingProgramToolBase {
    public:
        BrushTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox(Toolbar& t) override;
        virtual void gui_phone_toolbox(PhoneDrawingProgramScreen& t) override;
        virtual void tool_update() override;
        virtual void right_click_popup_gui(Toolbar& t, Vector2f popupPos) override;
        virtual void erase_component(CanvasComponentContainer::ObjInfo* erasedComp) override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        virtual void input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) override;
    private:
        bool extensive_point_checking_back(const Vector2f& newPoint);
        bool extensive_point_checking(const Vector2f& newPoint);
        void smooth_out_points(float smoothFactor);
        void fix_tip();
        void commit_stroke();
        void commit_data(bool final);

        float penWidth = 1.0f;
        bool addedTemporaryPoint = false;
        bool commitUpdate = false;

        std::vector<BrushComponentCode::BrushPoint> brushPoints;

        CanvasComponentContainer::ObjInfo* objInfoBeingEdited = nullptr;

        Vector2f prevPointUnaltered = {0, 0};
};
