#include "EraserTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
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
    drawP.world.main.toolConfig.relative_width_slider(t.gui, "Size", &drawP.world.main.toolConfig.eraser.relativeWidth);
    t.gui.text_label("Erase from:");
    if(t.gui.radio_button_field("layer edited", "Layer being edited", drawP.controls.layerSelector == DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED))
        drawP.controls.layerSelector = DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED;
    if(t.gui.radio_button_field("all visible", "All visible layers", drawP.controls.layerSelector == DrawingProgramLayerManager::LayerSelector::ALL_VISIBLE_LAYERS))
        drawP.controls.layerSelector = DrawingProgramLayerManager::LayerSelector::ALL_VISIBLE_LAYERS;
    t.gui.pop_id();
}

bool EraserTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
    return true;
}

void EraserTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    erasedComponents.erase(erasedComp);
}

void EraserTool::switch_tool(DrawingProgramToolType newTool) {
    drawP.layerMan.erase_component_container(erasedComponents);
}

void EraserTool::tool_update() {
    if(drawP.controls.cursorHoveringOverCanvas)
        drawP.world.main.input.hideCursor = true;

    if(drawP.controls.leftClickHeld) {
        const Vector2f& prevMousePos = drawP.controls.leftClick ? drawP.world.main.input.mouse.pos : drawP.world.main.input.mouse.lastPos;
        SCollision::ColliderCollection<float> cC;
        float width = drawP.world.main.toolConfig.get_relative_width(drawP.world.main.toolConfig.eraser.relativeWidth);
        SCollision::generate_wide_line(cC, prevMousePos, drawP.world.main.input.mouse.pos, width * 2.0f, true);
        auto cCWorld = drawP.world.drawData.cam.c.collider_to_world<SCollision::ColliderCollection<WorldScalar>, SCollision::ColliderCollection<float>>(cC);

        drawP.drawCache.traverse_bvh_run_function(cCWorld.bounds, [&](const auto& bvhNode) {
            if(bvhNode &&
               SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.min)) &&
               SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.max)) &&
               SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.top_right())) &&
               SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.bottom_left()))) {
                drawP.drawCache.invalidate_cache_at_aabb(bvhNode->bounds);
                drawP.drawCache.traverse_bvh_run_function_starting_at_node_no_collision_check(bvhNode, [&](const auto& bvhNodeChild) {
                    drawP.drawCache.node_loop_erase_if_components(bvhNodeChild, [&](auto c) {
                        if(drawP.layerMan.component_passes_layer_selector(c, drawP.controls.layerSelector)) {
                            erasedComponents.emplace(c);
                            return true;
                        }
                        return false;
                    });
                    return true;
                });
                return false;
            }
            drawP.drawCache.node_loop_erase_if_components(bvhNode, [&](auto c) {
                if(drawP.layerMan.component_passes_layer_selector(c, drawP.controls.layerSelector) && c->obj->collides_with(drawP.world.drawData.cam.c, cCWorld, cC)) {
                    erasedComponents.emplace(c);
                    drawP.drawCache.invalidate_cache_at_aabb(c->obj->get_world_bounds());
                    return true;
                }
                return false;
            });
            return true;
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
        SkColor4f c = drawP.world.canvasTheme.get_tool_front_color();
        c.fA = 0.5f;

        SkPaint linePaint;
        linePaint.setColor4f(c);
        linePaint.setStyle(SkPaint::kStroke_Style);
        linePaint.setStrokeCap(SkPaint::kRound_Cap);
        float width = drawP.world.main.toolConfig.get_relative_width(drawP.world.main.toolConfig.eraser.relativeWidth);
        linePaint.setStrokeWidth(width * 2.0f);
        SkPathBuilder erasePath;
        erasePath.moveTo(convert_vec2<SkPoint>(drawData.main->input.mouse.lastPos));
        erasePath.lineTo(convert_vec2<SkPoint>(drawData.main->input.mouse.pos));
        canvas->drawPath(erasePath.detach(), linePaint);
    }
}
