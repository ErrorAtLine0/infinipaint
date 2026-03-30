#include "EraserTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include <include/core/SkBlendMode.h>
#include <include/core/SkPathBuilder.h>

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"

EraserTool::EraserTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType EraserTool::get_type() {
    return DrawingProgramToolType::ERASER;
}

void EraserTool::gui_toolbox() {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("eraser tool");
    text_label_centered(t.gui, "Eraser");
    drawP.world.main.toolConfig.relative_width_gui(drawP, "Size");
    text_label(t.gui, "Erase from:");
    radio_button_selector(t.gui, "layer selector", &drawP.controls.layerSelector, {
        {"Layer being edited", DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED},
        {"All visible layers", DrawingProgramLayerManager::LayerSelector::ALL_VISIBLE_LAYERS}
    });
    t.gui.pop_id();
}

void EraserTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
}

void EraserTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        if(button.down && !isErasing) {
            isErasing = true;
            erase_between_points(button.pos, button.pos);
        }
        else if(!button.down && isErasing)
            switch_tool(get_type());
    }
}

void EraserTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(isErasing)
        erase_between_points(motion.pos, motion.pos - motion.move);
}

void EraserTool::erase_between_points(const Vector2f& start, const Vector2f& end) {
    auto relativeWidthResult = drawP.world.main.toolConfig.get_relative_width_stroke_size(drawP, drawP.world.drawData.cam.c.inverseScale);
    if(!relativeWidthResult.first.has_value()) {
        drawP.world.main.toolConfig.print_relative_width_fail_message(relativeWidthResult.second);
        isErasing = false;
        return;
    }
    float width = relativeWidthResult.first.value() * 0.5f;

    SCollision::ColliderCollection<float> cC;
    SCollision::generate_wide_line(cC, start, end, width * 2.0f, true);
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

void EraserTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    erasedComponents.erase(erasedComp);
}

void EraserTool::switch_tool(DrawingProgramToolType newTool) {
    drawP.layerMan.erase_component_container(erasedComponents);
    isErasing = false;
}

void EraserTool::tool_update() {
    if(drawP.controls.cursorHoveringOverCanvas)
        drawP.world.main.input.hideCursor = true;
}

bool EraserTool::prevent_undo_or_redo() {
    return drawP.controls.leftClickHeld;
}

void EraserTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(!drawP.world.main.input.isTouchDevice && drawData.main->world->drawProg.controls.cursorHoveringOverCanvas) {
        if(!lastPosOpt.has_value())
            lastPosOpt = drawData.main->input.mouse.pos;
        auto& lastPos = lastPosOpt.value();

        auto relativeWidthResult = drawP.world.main.toolConfig.get_relative_width_stroke_size(drawP, drawP.world.drawData.cam.c.inverseScale);
        if(relativeWidthResult.first.has_value()) {
            float width = relativeWidthResult.first.value() * 0.5f;
            SkPaint linePaint;
            linePaint.setAntiAlias(drawData.skiaAA);
            linePaint.setColor4f({1.0f, 1.0f, 1.0f, 0.5f});
            linePaint.setStyle(SkPaint::kStroke_Style);
            linePaint.setStrokeCap(SkPaint::kRound_Cap);
            linePaint.setStrokeWidth(width * 2.0f);
            linePaint.setBlender(CanvasTheme::get_visible_blend_mode());
            SkPathBuilder erasePath;
            erasePath.moveTo(convert_vec2<SkPoint>(lastPos));
            erasePath.lineTo(convert_vec2<SkPoint>(drawData.main->input.mouse.pos));
            canvas->drawPath(erasePath.detach(), linePaint);
        }

        lastPos = drawData.main->input.mouse.pos;
    }
    else
        lastPosOpt = std::nullopt;
}
