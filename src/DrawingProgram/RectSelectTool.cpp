#include "RectSelectTool.hpp"
#include <chrono>
#include "DrawingProgram.hpp"
#include "../Server/CommandList.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "Helpers/FixedPoint.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "Helpers/Serializers.hpp"
#include "../CoordSpaceHelper.hpp"
#include <cereal/types/vector.hpp>
#include <algorithm>
#include <ranges>

#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include <include/gpu/graphite/Surface.h>
#elif USE_SKIA_BACKEND_GANESH
    #include <include/gpu/ganesh/GrDirectContext.h>
    #include <include/gpu/ganesh/SkSurfaceGanesh.h>
#endif

#define ROTATION_POINT_RADIUS_MULTIPLIER 0.7
#define ROTATION_POINTS_DISTANCE 20.0

RectSelectTool::RectSelectTool(DrawingProgram& initDrawP):
    drawP(initDrawP)
{
}

void RectSelectTool::gui_toolbox() {
    drawP.world.main.toolbar.gui.text_label_centered("Rectangle Select");
}

void RectSelectTool::reset_tool() {
    drawP.selection.deselect_all();
    controls = RectSelectControls();
    drawP.compCache.disableRefresh = false;
}

void RectSelectTool::tool_update() {
    switch(controls.selectionMode) {
        case 0: {
            drawP.compCache.disableRefresh = false;
            if(drawP.controls.leftClick) {
                if(drawP.selection.is_something_selected()) {
                    if(!drawP.selection.mouse_collided_with_selection_aabb())
                        reset_tool();
                }
                else {
                    reset_tool();
                    controls.coords = drawP.world.drawData.cam.c;
                    controls.selectStartAt = controls.coords.get_mouse_pos(drawP.world);
                    controls.selectionMode = 1;
                }
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

                drawP.selection.add_from_cam_coord_collider_to_selection(cC);

                controls.selectionMode = 0;
                drawP.compCache.disableRefresh = true;
            }
            break;
        }
    }
}

void RectSelectTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(controls.selectionMode == 1) {
        canvas->save();
        controls.coords.transform_sk_canvas(canvas, drawData);

        SkPaint selectionPaint;
        selectionPaint.setColor4f({0.3f, 0.6f, 0.9f, 0.4f});

        SkRect r = SkRect::MakeLTRB(controls.newT[0].x(), controls.newT[0].y(), controls.newT[2].x(), controls.newT[2].y());
        canvas->drawRect(r, selectionPaint);
        canvas->restore();

        canvas->restore();
    }
}
