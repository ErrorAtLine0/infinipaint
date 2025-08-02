#include "LassoSelectTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "../CoordSpaceHelper.hpp"
#include <ranges>
#include <delaunator.hpp>

LassoSelectTool::LassoSelectTool(DrawingProgram& initDrawP):
    drawP(initDrawP)
{
}

void LassoSelectTool::gui_toolbox() {
    drawP.world.main.toolbar.gui.text_label_centered("Lasso Select");
}

void LassoSelectTool::reset_tool() {
    drawP.selection.deselect_all();
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
            if(vec_distance(controls.lassoPoints.back(), newLassoPoint)) {
                controls.lassoPoints.emplace_back(newLassoPoint);
            }
            if(!drawP.controls.leftClickHeld) {
                std::vector<double> dCoords;
                for(Vector2f& lassoPoint : controls.lassoPoints) {
                    dCoords.emplace_back(lassoPoint.x());
                    dCoords.emplace_back(lassoPoint.y());
                }
                delaunator::Delaunator d(dCoords);
                ColliderCollection<float> cC;
                for(size_t i = 0; i < d.triangles.size(); i += 3) {
                    SCollision::Triangle tri{
                        Vector2f{dCoords[2 * d.triangles[i]], dCoords[2 * d.triangles[i] + 1]},
                        Vector2f{dCoords[2 * d.triangles[i + 1]], dCoords[2 * d.triangles[i + 1] + 1]},
                        Vector2f{dCoords[2 * d.triangles[i + 2]], dCoords[2 * d.triangles[i + 2] + 1]}
                    };
                    cC.triangle.emplace_back(tri);
                }
                drawP.selection.add_from_cam_coord_collider_to_selection(cC);
                controls.selectionMode = 0;
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
