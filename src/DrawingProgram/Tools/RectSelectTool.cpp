#include "RectSelectTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "../../CoordSpaceHelper.hpp"

RectSelectTool::RectSelectTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{
}

DrawingProgramToolType RectSelectTool::get_type() {
    return DrawingProgramToolType::RECTSELECT;
}

void RectSelectTool::gui_toolbox() {
    auto& t = drawP.world.main.toolbar;
    t.gui.push_id("rectangle select tool");
    t.gui.text_label_centered("Rectangle Select");
    drawP.selection.selection_gui();
    t.gui.pop_id();
}

void RectSelectTool::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    drawP.selection.input_key_callback_display_selection(key);
    drawP.selection.input_key_callback_modify_selection(key);
}

void RectSelectTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    drawP.selection.input_mouse_button_on_canvas_callback_modify_selection(button);
    if(!controls.isSelecting && button.button == InputManager::MouseButton::LEFT && button.down && !drawP.selection.is_being_transformed()) {
        controls = RectSelectControls();
        controls.coords = drawP.world.drawData.cam.c;
        controls.selectStartAt = controls.selectEndAt = button.pos;
        controls.isSelecting = true;
    }
    else if(controls.isSelecting && button.button == InputManager::MouseButton::LEFT && !button.down) {
        using namespace SCollision;
        controls.selectEndAt = controls.coords.to_space(drawP.world.drawData.cam.c.from_space(button.pos));
        Vector2f newP1 = drawP.world.drawData.cam.c.to_space(controls.coords.from_space(cwise_vec_min(controls.selectStartAt, controls.selectEndAt)));
        Vector2f newP2 = drawP.world.drawData.cam.c.to_space(controls.coords.from_space(cwise_vec_max(controls.selectStartAt, controls.selectEndAt)));
        std::array<Vector2f, 4> newT = triangle_from_rect_points(newP1, newP2);

        ColliderCollection<float> cC;
        cC.triangle.emplace_back(newT[0], newT[1], newT[2]);
        cC.triangle.emplace_back(newT[2], newT[3], newT[0]);
        cC.recalculate_bounds();

        if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held)
            drawP.selection.add_from_cam_coord_collider_to_selection(cC, drawP.controls.layerSelector, false);
        else if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LALT).held)
            drawP.selection.remove_from_cam_coord_collider_to_selection(cC, drawP.controls.layerSelector, false);
        else {
            drawP.selection.deselect_all();
            drawP.selection.add_from_cam_coord_collider_to_selection(cC, drawP.controls.layerSelector, false);
        }

        controls.isSelecting = false;
    }
}

void RectSelectTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(controls.isSelecting)
        controls.selectEndAt = controls.coords.to_space(drawP.world.drawData.cam.c.from_space(motion.pos));
    drawP.selection.input_mouse_motion_callback_modify_selection(motion);
}

void RectSelectTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
}

bool RectSelectTool::right_click_popup_gui(Vector2f popupPos) {
    return drawP.selection_action_menu(popupPos);
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

void RectSelectTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(controls.isSelecting) {
        Vector2f mPos = controls.selectEndAt;
        Vector2f newP1 = cwise_vec_min(controls.selectStartAt, mPos);
        Vector2f newP2 = cwise_vec_max(controls.selectStartAt, mPos);
        std::array<Vector2f, 4> newT = triangle_from_rect_points(newP1, newP2);

        canvas->save();
        controls.coords.transform_sk_canvas(canvas, drawData);

        SkRect r = SkRect::MakeLTRB(newT[0].x(), newT[0].y(), newT[2].x(), newT[2].y());
        canvas->drawRect(r, drawP.select_tool_line_paint());

        canvas->restore();
    }
}
