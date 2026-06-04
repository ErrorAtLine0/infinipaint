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

#include "RectSelectTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "../../CoordSpaceHelper.hpp"

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"

RectSelectTool::RectSelectTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{
}

DrawingProgramToolType RectSelectTool::get_type() {
    return DrawingProgramToolType::RECTSELECT;
}

void RectSelectTool::gui_toolbox(Toolbar& t) {
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("rectangle select tool", [&] {
        GUIStuff::ElementHelpers::text_label_centered(gui, "Rectangle Select");
        drawP.selection.selection_gui(t);
    });
}

void RectSelectTool::gui_phone_toolbox(PhoneDrawingProgramScreen& t) {
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("rect select tool", [&] {
        drawP.selection.phone_selection_gui(t);
    });
}

void RectSelectTool::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    drawP.selection.input_key_callback_modify_selection(key);
}

void RectSelectTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    drawP.selection.input_mouse_button_on_canvas_callback_modify_selection(button);
    if(!controls.isSelecting && button.button == InputManager::MouseButton::LEFT && button.down && !drawP.selection.is_being_transformed() && !drawP.world.main.g.gui.cursor_obstructed()) {
        controls = RectSelectControls();
        controls.coords = drawP.world.drawData.cam.c;
        controls.selectStartAt = controls.selectEndAt = button.pos;
        controls.isSelecting = true;
    }
    else if(controls.isSelecting && button.button == InputManager::MouseButton::LEFT && !button.down) {
        using namespace SCollision;
        controls.selectEndAt = controls.coords.from_cam_space_to_this(drawP.world, button.pos);
        Vector2f newP1 = controls.coords.from_this_to_cam_space(drawP.world, cwise_vec_min(controls.selectStartAt, controls.selectEndAt));
        Vector2f newP2 = controls.coords.from_this_to_cam_space(drawP.world, cwise_vec_max(controls.selectStartAt, controls.selectEndAt));

        SkPath cC = SkPath::Rect(SkRect::MakeLTRB(newP1.x(), newP1.y(), newP2.x(), newP2.y()));

        if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held)
            drawP.selection.add_from_cam_coord_collider_to_selection(cC, drawP.controls.layerSelector, false);
        else if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LALT).held)
            drawP.selection.remove_from_cam_coord_collider_to_selection(cC, drawP.controls.layerSelector, false);
        else {
            drawP.selection.deselect_all();
            drawP.selection.add_from_cam_coord_collider_to_selection(cC, drawP.controls.layerSelector, false);
        }

        controls.isSelecting = false;
        drawP.world.main.g.gui.set_to_layout();
    }
}

void RectSelectTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(controls.isSelecting)
        controls.selectEndAt = controls.coords.from_cam_space_to_this(drawP.world, motion.pos);
    drawP.selection.input_mouse_motion_callback_modify_selection(motion);
}

void RectSelectTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
}

void RectSelectTool::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    drawP.selection_action_menu(popupPos);
}

void RectSelectTool::switch_tool(DrawingProgramToolType newTool) {
    if(!drawP.is_selection_allowing_tool(newTool))
        drawP.selection.deselect_all();
}

void RectSelectTool::tool_update() {
}

bool RectSelectTool::prevent_undo_or_redo() {
    return drawP.selection.is_something_selected() || controls.isSelecting;
}

Vector4f* RectSelectTool::color_picker_color(Vector4f* oldColor) {
    return drawP.selection.color_picker_color(oldColor);
}

void RectSelectTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(controls.isSelecting) {
        Vector2f mPos = controls.selectEndAt;
        Vector2f newP1 = cwise_vec_min(controls.selectStartAt, mPos);
        Vector2f newP2 = cwise_vec_max(controls.selectStartAt, mPos);
        std::array<Vector2f, 4> newT = triangle_from_rect_points(newP1, newP2);

        canvas->save();
        controls.coords.transform_sk_canvas(canvas, drawData);

        SkRect r = SkRect::MakeLTRB(newT[0].x(), newT[0].y(), newT[2].x(), newT[2].y());

        auto paintPair = drawP.select_tool_line_paint(drawData);
        canvas->drawRect(r, paintPair.first);
        canvas->drawRect(r, paintPair.second);

        canvas->restore();
    }
}
