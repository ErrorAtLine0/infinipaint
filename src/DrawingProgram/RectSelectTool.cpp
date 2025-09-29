#include "RectSelectTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "../CoordSpaceHelper.hpp"

RectSelectTool::RectSelectTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{
}

DrawingProgramToolType RectSelectTool::get_type() {
    return DrawingProgramToolType::RECTSELECT;
}

void RectSelectTool::gui_toolbox() {
    drawP.world.main.toolbar.gui.text_label_centered("Rectangle Select");
}

void RectSelectTool::switch_tool(DrawingProgramToolType newTool) {
    if(!drawP.is_selection_tool(newTool))
        drawP.selection.deselect_all();
}

void RectSelectTool::tool_update() {
    switch(controls.selectionMode) {
        case 0: {
            if(drawP.controls.leftClick && !drawP.selection.is_being_transformed()) {
                controls = RectSelectControls();
                controls.coords = drawP.world.drawData.cam.c;
                controls.selectStartAt = controls.coords.get_mouse_pos(drawP.world);
                controls.selectionMode = 1;
            }
            break;
        }
        case 1: {
            using namespace SCollision;
            Vector2f mPos = controls.coords.get_mouse_pos(drawP.world);
            Vector2f newP1 = cwise_vec_min(controls.selectStartAt, mPos);
            Vector2f newP2 = cwise_vec_max(controls.selectStartAt, mPos);
            controls.newT = triangle_from_rect_points(newP1, newP2);
            if(!drawP.controls.leftClickHeld) {
                ColliderCollection<float> cC;
                cC.triangle.emplace_back(controls.newT[0], controls.newT[1], controls.newT[2]);
                cC.triangle.emplace_back(controls.newT[2], controls.newT[3], controls.newT[0]);
                cC.recalculate_bounds();

                if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held)
                    drawP.selection.add_from_cam_coord_collider_to_selection(cC);
                else if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LALT).held)
                    drawP.selection.remove_from_cam_coord_collider_to_selection(cC);
                else {
                    drawP.selection.deselect_all();
                    drawP.selection.add_from_cam_coord_collider_to_selection(cC);
                }

                controls.selectionMode = 0;
            }
            break;
        }
    }
}

bool RectSelectTool::prevent_undo_or_redo() {
    return drawP.selection.is_something_selected() || controls.selectionMode == 1;
}

void RectSelectTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(controls.selectionMode == 1) {
        canvas->save();
        controls.coords.transform_sk_canvas(canvas, drawData);

        SkRect r = SkRect::MakeLTRB(controls.newT[0].x(), controls.newT[0].y(), controls.newT[2].x(), controls.newT[2].y());
        canvas->drawRect(r, drawP.select_tool_line_paint());
        canvas->restore();

        canvas->restore();
    }
}
