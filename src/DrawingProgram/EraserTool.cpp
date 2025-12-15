#include "EraserTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include <include/core/SkPathBuilder.h>

EraserTool::EraserTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType EraserTool::get_type() {
    return DrawingProgramToolType::ERASER;
}

void EraserTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("eraser tool");
    t.gui.text_label_centered("Eraser");
    t.gui.slider_scalar_field("relwidth", "Size", &drawP.controls.relativeWidth, 3.0f, 40.0f);
    t.gui.pop_id();
}

bool EraserTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
    return true;
}

void EraserTool::erase_component(const CanvasComponentContainer::ObjInfoSharedPtr& erasedComp) {
    erasedComponents.erase(erasedComp);
}

void EraserTool::switch_tool(DrawingProgramToolType newTool) {
    DrawingProgramCache::move_components_from_bvh_nodes_to_set(erasedComponents, erasedBVHNodes);
    if(!erasedComponents.empty()) {
        drawP.erase_component_set(erasedComponents);
        // NOTE: Ideally, you would just recalculate the lastDrawnPos of every cache instead of invalidating and redrawing the caches
        // However, the eraser might have other visual glitches during drawing that i am unaware of, so for now, just refresh the
        // cache after a draw
        drawP.compCache.clear_own_cached_surfaces();
    }
    erasedComponents.clear();
    erasedBVHNodes.clear();
}

void EraserTool::tool_update() {
    if(drawP.controls.cursorHoveringOverCanvas)
        drawP.world.main.input.hideCursor = true;

    if(drawP.controls.leftClickHeld) {
        const Vector2f& prevMousePos = drawP.controls.leftClick ? drawP.world.main.input.mouse.pos : drawP.world.main.input.mouse.lastPos;
        SCollision::ColliderCollection<float> cC;
        SCollision::generate_wide_line(cC, prevMousePos, drawP.world.main.input.mouse.pos, drawP.controls.relativeWidth * 2.0f, true);
        auto cCWorld = drawP.world.drawData.cam.c.collider_to_world<SCollision::ColliderCollection<WorldScalar>, SCollision::ColliderCollection<float>>(cC);

        drawP.compCache.traverse_bvh_erase_function(cCWorld.bounds, [&](const auto& bvhNode, auto& comps) {
            if(bvhNode &&
               SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.min)) &&
               SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.max)) &&
               SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.top_right())) &&
               SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.bottom_left()))) {
                erasedBVHNodes.emplace(bvhNode);
                drawP.compCache.invalidate_cache_at_aabb_before_pos(bvhNode->bounds, 0);
                return true;
            }
            std::erase_if(comps, [&](auto& c) {
                if(c->obj->collides_with(drawP.world.drawData.cam.c, cCWorld, cC)) {
                    erasedComponents.emplace(c);
                    drawP.compCache.invalidate_cache_at_aabb_before_pos(c->obj->get_world_bounds(), c->pos);
                    return true;
                }
                return false;
            });
            return false;
        });
    }
    else
        switch_tool(get_type());
}

bool EraserTool::prevent_undo_or_redo() {
    return drawP.controls.leftClickHeld;
}

void EraserTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(!drawData.main->toolbar.io->hoverObstructed) {
        SkColor4f c = drawP.world.canvasTheme.toolFrontColor;
        c.fA = 0.5f;

        SkPaint linePaint;
        linePaint.setColor4f(c);
        linePaint.setStyle(SkPaint::kStroke_Style);
        linePaint.setStrokeCap(SkPaint::kRound_Cap);
        linePaint.setStrokeWidth(drawP.controls.relativeWidth * 2.0f);
        SkPathBuilder erasePath;
        erasePath.moveTo(convert_vec2<SkPoint>(drawData.main->input.mouse.lastPos));
        erasePath.lineTo(convert_vec2<SkPoint>(drawData.main->input.mouse.pos));
        canvas->drawPath(erasePath.detach(), linePaint);
    }
}
