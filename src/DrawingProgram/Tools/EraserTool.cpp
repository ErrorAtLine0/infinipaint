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

#include "EraserTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include <include/core/SkBlendMode.h>
#include <include/core/SkPathBuilder.h>
#include "../../CanvasComponents/BrushComponentCode.hpp"
#include "../EditCanvasComponentWorldUndoAction.hpp"

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/CheckBoxHelpers.hpp"

EraserTool::EraserTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType EraserTool::get_type() {
    return DrawingProgramToolType::ERASER;
}

void EraserTool::gui_toolbox(Toolbar& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;
    gui.new_id("eraser tool", [&] {
        text_label_centered(gui, "Eraser");
        drawP.world.main.toolConfig.relative_width_gui(drawP, "Size");
        text_label(gui, "Erase from:");
        radio_button_selector(gui, "layer selector", &drawP.controls.layerSelector, {
            {"Layer being edited", DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED},
            {"All visible layers", DrawingProgramLayerManager::LayerSelector::ALL_VISIBLE_LAYERS}
        });
        checkbox_boolean_field(gui, "erase details", "Erase details", &drawP.world.main.toolConfig.eraser.eraseDetail);
    });
}

void EraserTool::gui_phone_toolbox(PhoneDrawingProgramScreen& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;

    gui.new_id("eraser tool", [&] {
        drawP.world.main.toolConfig.relative_width_gui(drawP, "Size");
        text_label(gui, "Erase from:");
        radio_button_selector(gui, "layer selector", &drawP.controls.layerSelector, {
            {"Layer being edited", DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED},
            {"All visible layers", DrawingProgramLayerManager::LayerSelector::ALL_VISIBLE_LAYERS}
        });
        checkbox_boolean_field(gui, "erase details", "Erase details", &drawP.world.main.toolConfig.eraser.eraseDetail);
    });
}

void EraserTool::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    t.paint_popup(popupPos);
}

void EraserTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        if(button.down && !isErasing && !drawP.world.main.g.gui.cursor_obstructed()) {
            isErasing = true;
            start = end = button.pos;
        }
        else if(!button.down && isErasing)
            switch_tool(get_type());
    }
}

void EraserTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    end = motion.pos;
}

void EraserTool::erase_between_points() {
    auto cCWorldBounds = drawP.world.drawData.cam.c.collider_to_world<SCollision::AABB<WorldScalar>, SCollision::AABB<float>>(erasePath.getBounds());
    drawP.drawCache.traverse_bvh_run_function(cCWorldBounds, [&](const auto& bvhNode) {
        if(bvhNode &&
           erasePath.contains(convert_vec2<SkPoint>(drawP.world.drawData.cam.c.to_space(bvhNode->bounds.min))) &&
           erasePath.contains(convert_vec2<SkPoint>(drawP.world.drawData.cam.c.to_space(bvhNode->bounds.max))) &&
           erasePath.contains(convert_vec2<SkPoint>(drawP.world.drawData.cam.c.to_space(bvhNode->bounds.top_right()))) &&
           erasePath.contains(convert_vec2<SkPoint>(drawP.world.drawData.cam.c.to_space(bvhNode->bounds.bottom_left())))) {
            drawP.drawCache.invalidate_cache_at_aabb(bvhNode->bounds);
            drawP.drawCache.traverse_bvh_run_function_starting_at_node_no_collision_check(bvhNode, [&](const auto& bvhNodeChild) {
                drawP.drawCache.node_loop_erase_if_components(bvhNodeChild, [&](auto c) {
                    if(drawP.layerMan.component_passes_layer_selector(c, drawP.controls.layerSelector)) {
                        auto it = updatedComponents.find(c);
                        if(it != updatedComponents.end()) {
                            c->obj->get_comp().set_data_from(*(it->second));
                            updatedComponents.erase(it);
                        }
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
            if(drawP.world.main.toolConfig.eraser.eraseDetail) {
                if(drawP.layerMan.component_passes_layer_selector(c, drawP.controls.layerSelector)) {
                    auto dataCopy = c->obj->get_comp().get_data_copy();
                    CanvasComponentEraseDetailResult result = c->obj->collides_with_erase_detail(drawP.world.drawData.cam.c, erasePath);
                    switch(result) {
                        case CanvasComponentEraseDetailResult::NO_CHANGE:
                            return false;
                        case CanvasComponentEraseDetailResult::CHANGED: {
                            if(!updatedComponents.contains(c))
                                updatedComponents.emplace(c, std::move(dataCopy));
                            c->obj->commit_update(drawP);
                            return false;
                        }
                        case CanvasComponentEraseDetailResult::REMOVED:
                            auto it = updatedComponents.find(c);
                            if(it != updatedComponents.end()) {
                                c->obj->get_comp().set_data_from(*(it->second));
                                updatedComponents.erase(it);
                            }
                            erasedComponents.emplace(c);
                            drawP.drawCache.invalidate_cache_at_optional_aabb(c->obj->get_world_bounds());
                            return true;
                    }
                }
            }
            else {
                if(drawP.layerMan.component_passes_layer_selector(c, drawP.controls.layerSelector) && c->obj->collides_with(drawP.world.drawData.cam.c, erasePath)) {
                    erasedComponents.emplace(c);
                    drawP.drawCache.invalidate_cache_at_optional_aabb(c->obj->get_world_bounds());
                    return true;
                }
            }
            return false;
        });
        return true;
    });
}

void EraserTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    erasedComponents.erase(erasedComp);
    updatedComponents.erase(erasedComp);
}

void EraserTool::switch_tool(DrawingProgramToolType newTool) {
    drawP.layerMan.erase_component_container(erasedComponents);
    bool first = true;
    for(auto& [comp, oldData] : updatedComponents) {
        comp->obj->send_comp_update(drawP, true);
        if(erasedComponents.empty() && first) {
            first = false;
            drawP.world.undo.push(std::make_unique<EditCanvasComponentWorldUndoAction>(std::move(oldData), drawP.world.undo.get_undoid_from_netid(comp->obj.get_net_id())));
        }
        else
            drawP.world.undo.push_on_last(std::make_unique<EditCanvasComponentWorldUndoAction>(std::move(oldData), drawP.world.undo.get_undoid_from_netid(comp->obj.get_net_id())));
    }
    erasedComponents.clear();
    updatedComponents.clear();
    isErasing = false;
}

void EraserTool::tool_update() {
    if(!drawP.world.main.g.gui.cursor_obstructed())
        drawP.world.main.input.hideCursor = true;

    auto relativeWidthResult = drawP.world.main.toolConfig.get_relative_width_stroke_size(drawP, drawP.world.drawData.cam.c.inverseScale);
    if(!relativeWidthResult.first.has_value()) {
        isErasing = false;
        erasePath = eraseBorderPath = SkPath();
        return;
    }

    float width = relativeWidthResult.first.value() * 0.5f;
    using namespace BrushComponentCode;
    std::vector<BrushPoint> brushPoints;
    brushPoints.emplace_back(start, width);
    if(start != end)
        brushPoints.emplace_back(end, width);
    erasePath = brush_stroke_to_skpath(brushPoints, true);
    brushPoints.clear();
    brushPoints.emplace_back(start, width + 1.0f);
    if(start != end)
        brushPoints.emplace_back(end, width + 1.0f);
    eraseBorderPath = brush_stroke_to_skpath(brushPoints, true);

    if(isErasing)
        erase_between_points();

    start = end;
}

bool EraserTool::prevent_undo_or_redo() {
    return drawP.controls.leftClickHeld;
}

void EraserTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(!drawP.world.main.input.isTouchDevice && !drawData.main->g.gui.cursor_obstructed() && !erasePath.isEmpty() && !eraseBorderPath.isEmpty()) {
        SkPaint linePaint;
        linePaint.setAntiAlias(drawData.skiaAA);
        linePaint.setColor4f({0.0f, 0.0f, 0.0f, 0.4f});
        linePaint.setStyle(SkPaint::kFill_Style);
        canvas->drawPath(erasePath, linePaint);

        linePaint.setColor4f({1.0f, 1.0f, 1.0f, 0.4f});
        canvas->drawPath(eraseBorderPath, linePaint);
    }
}
