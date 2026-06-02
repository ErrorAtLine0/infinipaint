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
#include <include/pathops/SkPathOps.h>

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/CheckBoxHelpers.hpp"
#include "Helpers/NetworkingObjects/NetObjOrderedList.hpp"
#include "Helpers/Parallel.hpp"

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
            auto relativeWidthResult = drawP.world.main.toolConfig.get_relative_width_stroke_size(drawP, drawP.world.drawData.cam.c.inverseScale);
            if(!relativeWidthResult.first.has_value()) {
                drawP.world.main.toolConfig.print_relative_width_fail_message(relativeWidthResult.second);
                return;
            }

            BrushComponentCode::mouse_button(drawP, genData, drawP.world.drawData.cam.c, button, relativeWidthResult.first.value());
            // NOTE: Must set erase path when isErasing is set to true to make sure that the Circle path from not erasing part doesn't reach the erase_on_path code
            erasePath = BrushComponentCode::brush_stroke_to_skpath(genData.brushPoints, drawP.world.main.toolConfig.brush.hasRoundCaps);
            isErasing = true;
        }
        else if(!button.down && isErasing) {
            // If not real time eraser, we can benefit from simplifying the path
            std::optional<SkPath> simplified = Simplify(erasePath);
            if(simplified.has_value())
                erasePath = simplified.value();
            erase_on_path();
            reset_erasing_stroke();
            commit_erase();
            isErasing = false;
        }
    }
}

void EraserTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(isErasing) {
        auto& toolConfig = drawP.world.main.toolConfig;
        BrushComponentCode::mouse_motion(drawP, genData, motion.pos, toolConfig.get_relative_width_stroke_size(drawP, genData.coords.inverseScale).first.value());
    }
}

void EraserTool::input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) {
    if(axis.axis == SDL_PEN_AXIS_PRESSURE && drawP.world.main.conf.tabletOptions.pressureAffectsBrushWidth) {
        genData.penWidth = axis.value;
        if(genData.penWidth != 0.0f && isErasing) {
            auto& toolConfig = drawP.world.main.toolConfig;
            float width = toolConfig.get_relative_width_stroke_size(drawP, genData.coords.inverseScale).first.value();
            BrushComponentCode::pen_pressure(drawP, genData, width);
        }
    }
}

void EraserTool::erase_on_path() {
    auto cCWorldBounds = genData.coords.collider_to_world<SCollision::AABB<WorldScalar>, SCollision::AABB<float>>(erasePath.getBounds());
    WorldScalar eraseScaleToCheckAgainst = WorldScalar(drawP.world.main.toolConfig.get_relative_width_stroke_size(drawP, genData.coords.inverseScale).first.value()) * genData.coords.inverseScale;
    if(eraseScaleToCheckAgainst == WorldScalar(0))
        return;
    drawP.drawCache.traverse_bvh_run_function(cCWorldBounds, [&](const auto& bvhNode) {
        if(bvhNode &&
           erasePath.contains(convert_vec2<SkPoint>(genData.coords.to_space(bvhNode->bounds.min))) &&
           erasePath.contains(convert_vec2<SkPoint>(genData.coords.to_space(bvhNode->bounds.max))) &&
           erasePath.contains(convert_vec2<SkPoint>(genData.coords.to_space(bvhNode->bounds.top_right()))) &&
           erasePath.contains(convert_vec2<SkPoint>(genData.coords.to_space(bvhNode->bounds.bottom_left())))) {
            drawP.drawCache.invalidate_cache_at_aabb(bvhNode->bounds);
            drawP.drawCache.traverse_bvh_run_function_starting_at_node_no_collision_check(bvhNode, [&](const auto& bvhNodeChild) {
                drawP.drawCache.node_loop_erase_if_components(bvhNodeChild, [&](auto c) {
                    if(drawP.layerMan.component_passes_layer_selector(c, drawP.controls.layerSelector)) {
                        auto it = updatedComponents.find(c);
                        if(it != updatedComponents.end()) {
                            c->obj->get_comp().set_data_from(*it->second.copyData->obj);
                            c->obj->set_object_update_lock(drawP, false);
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
                    auto dataCopy = c->obj->get_data_copy();
                    CanvasComponentEraseDetailResult result = c->obj->collides_with_erase_detail(genData.coords, eraseScaleToCheckAgainst, erasePath);
                    switch(result) {
                        case CanvasComponentEraseDetailResult::NO_CHANGE:
                            return false;
                        case CanvasComponentEraseDetailResult::CHANGED: {
                            if(!updatedComponents.contains(c)) {
                                c->obj->set_object_update_lock(drawP, true);
                                updatedComponents.emplace(c, std::move(dataCopy));
                            }
                            // NOTE: commit_update is what should be run here, but invalidate_cache is being run instead. This is for a few reasons:
                            // - Mesh initialize_draw_data does nothing, so commit_update isnt necessary
                            // - worldAABB only shrinks, which means that invalidate_cache_at_optional_aabb will invalidate a "good enough" space even if worldAABB isn't updated (which commit_update does)
                            // - commit_update can't be run inside a traverse_bvh_run_function call (causes segfault and other issues). If that were necessary, we would have to defer that call for after the traversal is done
                            // - commit_update will be run at switch_tool time instead
                            drawP.drawCache.invalidate_cache_at_optional_aabb(c->obj->get_world_bounds());
                            return false;
                        }
                        case CanvasComponentEraseDetailResult::REMOVED: {
                            auto it = updatedComponents.find(c);
                            if(it != updatedComponents.end()) {
                                c->obj->get_comp().set_data_from(*it->second.copyData->obj);
                                c->obj->set_object_update_lock(drawP, false);
                                updatedComponents.erase(it);
                            }
                            else
                                c->obj->get_comp().set_data_from(*dataCopy->obj);

                            erasedComponents.emplace(c);
                            drawP.drawCache.invalidate_cache_at_optional_aabb(c->obj->get_world_bounds());
                            return true;
                        }
                    }
                }
            }
            else {
                if(drawP.layerMan.component_passes_layer_selector(c, drawP.controls.layerSelector) && c->obj->collides_with(genData.coords, erasePath)) {
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

void EraserTool::reset_erasing_stroke() {
    if(!genData.brushPoints.empty()) {
        auto lastBrushPoint = genData.brushPoints.back();
        genData.brushPoints.clear();
        genData.brushPoints.emplace_back(lastBrushPoint);
        genData.prevPointUnaltered = genData.coords.get_mouse_pos(drawP.world);
        genData.addedTemporaryPoint = false;
    }
}

void EraserTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    erasedComponents.erase(erasedComp);
    updatedComponents.erase(erasedComp);
    erasedComp->obj->set_object_update_lock(drawP, false);
}

void EraserTool::commit_erase() {
    bool placedNewObject = false;
    std::unordered_map<DrawingProgramLayerListItem*, std::vector<std::pair<CanvasComponentContainer::ObjInfoIterator, CanvasComponentContainer*>>> newObjectsToPlace;

    parallel_loop_container_mutable(updatedComponents, [&](auto& compPair) {
        compPair.second.splitComps = compPair.first->obj->get_comp().attempt_split(drawP);
    });

    std::erase_if(updatedComponents, [&] (auto& p) {
        auto& [comp, oldData] = p;
        comp->obj->set_object_update_lock(drawP, false);
        if(!oldData.splitComps.empty()) {
            for(CanvasComponentContainer* c : oldData.splitComps)
                newObjectsToPlace[comp->obj->parentLayer].emplace_back(std::next(comp->obj->objInfo), c);
            placedNewObject = true;
            erasedComponents.emplace(comp);
            comp->obj->get_comp().set_data_from(*oldData.copyData->obj);
            return true;
        }
        return false;
    });

    drawP.world.send_reliable_multi_command_to_all([&]() {
        bool firstPlacement = true;
        for(auto& [layer, compMap] : newObjectsToPlace) {
            layer->get_layer().components->sort_netobj_ordered_list_iterator_insert_list(layer->get_layer().components, compMap);
            drawP.layerMan.add_many_components_to_layer(layer, compMap, firstPlacement);
            firstPlacement = false;
        }
        bool objectsToErase = !erasedComponents.empty();
        if(objectsToErase)
            drawP.layerMan.erase_component_container(erasedComponents, !placedNewObject);
        bool first = !objectsToErase && !placedNewObject;
        parallel_loop_container(updatedComponents, [&](auto& compPair) {
            auto& [comp, oldData] = compPair;
            comp->obj->get_comp().simplify_paths();
            comp->obj->normalize_object_coordinates();
        });
        for(auto& [comp, oldData] : updatedComponents) {
            comp->obj->commit_update_dont_invalidate_cache(drawP);
            drawP.send_transforms_for({comp});
            comp->obj->send_comp_update(drawP, true);
            if(first) {
                first = false;
                drawP.world.undo.push(std::make_unique<EditTransformCanvasComponentWorldUndoAction>(std::move(oldData.copyData->obj), oldData.copyData->coords, drawP.world.undo.get_undoid_from_netid(comp->obj.get_net_id())));
            }
            else
                drawP.world.undo.push_on_last(std::make_unique<EditTransformCanvasComponentWorldUndoAction>(std::move(oldData.copyData->obj), oldData.copyData->coords, drawP.world.undo.get_undoid_from_netid(comp->obj.get_net_id())));
        }
    });
    erasedComponents.clear();
    updatedComponents.clear();
}

void EraserTool::switch_tool(DrawingProgramToolType newTool) {
    commit_erase();
}

void EraserTool::commit_data() {
    if(isErasing) {
        erasePath = BrushComponentCode::brush_stroke_to_skpath(genData.brushPoints, drawP.world.main.toolConfig.brush.hasRoundCaps);
        if(drawP.world.main.conf.realTimeEraser) {
            erase_on_path();
            reset_erasing_stroke();
        }
    }
    else {
        auto relativeWidthResult = drawP.world.main.toolConfig.get_relative_width_stroke_size(drawP, drawP.world.drawData.cam.c.inverseScale);
        if(!relativeWidthResult.first.has_value()) {
            erasePath = SkPath();
            return;
        }
        float width = relativeWidthResult.first.value() * 0.5f;
        erasePath = SkPath::Circle(drawP.world.main.input.mouse.pos.x(), drawP.world.main.input.mouse.pos.y(), width);
    }
}

void EraserTool::tool_update() {
    if(!drawP.world.main.g.gui.cursor_obstructed())
        drawP.world.main.input.hideCursor = true;

    using namespace BrushComponentCode;

    commit_data();
}

bool EraserTool::prevent_undo_or_redo() {
    return drawP.controls.leftClickHeld;
}

void EraserTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(!drawP.world.main.input.isTouchDevice && (!drawData.main->g.gui.cursor_obstructed() || isErasing) && !erasePath.isEmpty()) {
        if(isErasing) {
            CanvasComponentContainer::TransformData drawTransform = CanvasComponentContainer::calculate_draw_transform(drawData.cam.c, genData.coords);
            canvas->save();
            CanvasComponentContainer::canvas_do_transform(canvas, drawTransform);
        }
        SkPaint linePaint;
        linePaint.setAntiAlias(drawData.skiaAA);
        linePaint.setColor4f({0.0f, 0.0f, 0.0f, 0.4f});
        linePaint.setStyle(SkPaint::kFill_Style);
        canvas->drawPath(erasePath, linePaint);

        linePaint.setColor4f({1.0f, 1.0f, 1.0f, 0.4f});
        canvas->drawPath(erasePath, linePaint);
        if(isErasing)
            canvas->restore();
    }
}
