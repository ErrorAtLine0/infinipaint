#include "LassoSelectTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "../../CoordSpaceHelper.hpp"
#include <ranges>
#include <earcut.hpp>
#include <include/core/SkPathBuilder.h>

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"

namespace mapbox {
namespace util {

template <>
struct nth<0, Vector2f> {
    inline static auto get(const Vector2f &t) {
        return t.x();
    };
};
template <>
struct nth<1, Vector2f> {
    inline static auto get(const Vector2f &t) {
        return t.y();
    };
};

} // namespace util
} // namespace mapbox

LassoSelectTool::LassoSelectTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType LassoSelectTool::get_type() {
    return DrawingProgramToolType::LASSOSELECT;
}

void LassoSelectTool::gui_toolbox(Toolbar& t) {
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("lasso select tool", [&] {
        GUIStuff::ElementHelpers::text_label_centered(gui, "Lasso Select");
        drawP.selection.selection_gui(t);
    });
}

void LassoSelectTool::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    drawP.selection_action_menu(popupPos);
}

void LassoSelectTool::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    drawP.selection.input_key_callback_modify_selection(key);
}

void LassoSelectTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    drawP.selection.input_mouse_button_on_canvas_callback_modify_selection(button);
    if(!controls.isSelecting && button.button == InputManager::MouseButton::LEFT && button.down && !drawP.selection.is_being_transformed() && !drawP.world.main.g.gui.cursor_obstructed()) {
        controls = LassoSelectControls();
        controls.coords = drawP.world.drawData.cam.c;
        controls.lassoPoints.emplace_back(controls.coords.get_mouse_pos(drawP.world));
        controls.isSelecting = true;
    }
    else if(controls.isSelecting && button.button == InputManager::MouseButton::LEFT && !button.down) {
        if(controls.lassoPoints.size() > 3) {
            SCollision::ColliderCollection<float> cC;

            std::vector<std::vector<Vector2f>> polygon;
            polygon.emplace_back(controls.lassoPoints);
            auto& poly = polygon[0];
            for(auto& v : poly)
                v = controls.coords.from_this_to_cam_space(drawP.world, v);

            std::vector<unsigned> indices = mapbox::earcut(polygon);

            for(size_t i = 0; i < indices.size(); i += 3) {
                cC.triangle.emplace_back(
                    poly[indices[i]],
                    poly[indices[i + 1]],
                    poly[indices[i + 2]]
                );
            }

            cC.recalculate_bounds();

            if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held)
                drawP.selection.add_from_cam_coord_collider_to_selection(cC, drawP.controls.layerSelector, false);
            else if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LALT).held)
                drawP.selection.remove_from_cam_coord_collider_to_selection(cC, drawP.controls.layerSelector, false);
            else {
                drawP.selection.deselect_all();
                drawP.selection.add_from_cam_coord_collider_to_selection(cC, drawP.controls.layerSelector, false);
            }
            controls.lassoPoints.clear();
        }
        else if(!drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held && !drawP.world.main.input.key(InputManager::KEY_GENERIC_LALT).held) {
            drawP.selection.deselect_all();
        }
        controls.isSelecting = false;
        drawP.world.main.g.gui.set_to_layout();
    }
}

void LassoSelectTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(controls.isSelecting) {
        Vector2f newLassoPoint = controls.coords.from_cam_space_to_this(drawP.world, motion.pos);
        float lassoPointDist = vec_distance(controls.lassoPoints.back(), newLassoPoint);
        if(lassoPointDist > 4.0f)
            controls.lassoPoints.emplace_back(newLassoPoint);
    }
    drawP.selection.input_mouse_motion_callback_modify_selection(motion);
}

void LassoSelectTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
}

void LassoSelectTool::switch_tool(DrawingProgramToolType newTool) {
    if(!drawP.is_selection_allowing_tool(newTool))
        drawP.selection.deselect_all();
}

void LassoSelectTool::tool_update() {
}

bool LassoSelectTool::prevent_undo_or_redo() {
    return drawP.selection.is_something_selected() || controls.isSelecting;
}

void LassoSelectTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(controls.isSelecting) {
        canvas->save();
        controls.coords.transform_sk_canvas(canvas, drawData);

        SkPathBuilder lassoPath;
        lassoPath.moveTo(convert_vec2<SkPoint>(controls.lassoPoints.front()));
        for(Vector2f& p : controls.lassoPoints | std::views::drop(1))
            lassoPath.lineTo(convert_vec2<SkPoint>(p));

        canvas->drawPath(lassoPath.detach(), drawP.select_tool_line_paint(drawData));

        canvas->restore();
    }
}
