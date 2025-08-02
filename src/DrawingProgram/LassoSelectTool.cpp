#include "LassoSelectTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "../CoordSpaceHelper.hpp"
#include <ranges>
#include "../DrawCollision.hpp"
#include <earcut.hpp>

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

void LassoSelectTool::gui_toolbox() {
    drawP.world.main.toolbar.gui.text_label_centered("Lasso Select");
}

void LassoSelectTool::reset_tool() {
    drawP.selection.deselect_all();
    controls.lassoPoints.clear();
    controls = LassoSelectControls();
}

void LassoSelectTool::tool_update() {
    switch(controls.selectionMode) {
        case 0: {
            if(drawP.controls.leftClick) {
                if(drawP.selection.is_something_selected()) {
                    if(!drawP.selection.mouse_collided_with_selection())
                        reset_tool();
                }
                else {
                    reset_tool();
                    controls.coords = drawP.world.drawData.cam.c;
                    controls.lassoPoints.emplace_back(controls.coords.get_mouse_pos(drawP.world));
                    controls.selectionMode = 1;
                }
            }
            break;
        }
        case 1: {
            using namespace SCollision;
            Vector2f newLassoPoint = controls.coords.get_mouse_pos(drawP.world);
            float lassoPointDist = vec_distance(controls.lassoPoints.back(), newLassoPoint);
            if(lassoPointDist > 4.0f)
                controls.lassoPoints.emplace_back(newLassoPoint);

            if(!drawP.controls.leftClickHeld) {
                if(controls.lassoPoints.size() > 3) {
                    SCollision::ColliderCollection<float> cC;

                    std::vector<std::vector<Vector2f>> polygon;
                    polygon.emplace_back(controls.lassoPoints);
                    auto& poly = polygon[0];

                    std::vector<unsigned> indices = mapbox::earcut(polygon);

                    for(size_t i = 0; i < indices.size(); i += 3) {
                        cC.triangle.emplace_back(
                            poly[indices[i]],
                            poly[indices[i + 1]],
                            poly[indices[i + 2]]
                        );
                    }

                    cC.recalculate_bounds();

                    drawP.selection.add_from_cam_coord_collider_to_selection(cC);

                    controls.selectionMode = 0;
                }
                else
                    reset_tool();
            }
            break;
        }
    }
}

bool LassoSelectTool::prevent_undo_or_redo() {
    return drawP.selection.is_something_selected() || controls.selectionMode == 1;
}

void LassoSelectTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(controls.selectionMode == 1) {
        canvas->save();
        controls.coords.transform_sk_canvas(canvas, drawData);

        SkPath lassoPath;
        lassoPath.moveTo(convert_vec2<SkPoint>(controls.lassoPoints.front()));
        for(Vector2f& p : controls.lassoPoints | std::views::drop(1))
            lassoPath.lineTo(convert_vec2<SkPoint>(p));

        canvas->drawPath(lassoPath, drawP.select_tool_line_paint());

        canvas->restore();
    }
}
