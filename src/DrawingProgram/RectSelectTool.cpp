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
    if(controls.selectionMode == 3)
        commit_transformation();
    controls = RectSelectControls();
}

void RectSelectTool::tool_update() {
    switch(controls.selectionMode) {
        case 0: {
            if(drawP.controls.leftClick) {
                reset_tool();
                controls.coords = drawP.world.drawData.cam.c;
                controls.selectStartAt = controls.coords.get_mouse_pos(drawP.world);
                controls.rotationPointAngle = 0.0;
                controls.selectionMode = 1;
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
                ColliderCollection<float> selectionCollection;
                selectionCollection.triangle.emplace_back(controls.newT[0], controls.newT[1], controls.newT[2]);
                selectionCollection.triangle.emplace_back(controls.newT[2], controls.newT[3], controls.newT[0]);
                drawP.check_all_collisions_transform(selectionCollection);
                for(auto& c : drawP.components.client_list())
                    c->obj->selected = c->obj->globalCollisionCheck;
                controls.selectionMode = create_selection_aabb() ? 2 : 0;
                controls.rotationCenter = controls.finalSelectionAABB.center();
            }
            break;
        }
        case 2: {
            Vector2f mPos = drawP.world.main.input.mouse.pos;
            bool copyHit = drawP.world.main.input.key(InputManager::KEY_COPY).pressed;
            bool cutHit = drawP.world.main.input.key(InputManager::KEY_CUT).pressed;
            bool deleteHit = drawP.world.main.input.key(InputManager::KEY_DRAW_DELETE).pressed;
            bool unselectHit = drawP.world.main.input.key(InputManager::KEY_DRAW_UNSELECT).pressed;

            if(drawP.controls.leftClick) {
                if(SCollision::collide(mPos, controls.scaleSelectionCircle)) {
                    controls.scaleStartAt = controls.finalSelectionAABB.max;
                    controls.scaleCenterPoint = controls.finalSelectionAABB.center();
                    set_selected_items();
                    controls.selectionMode = 4;
                }
                else if(SCollision::collide(mPos, controls.rotationCenterCircle)) {
                    controls.moveStartAt = controls.rotationCenter;
                    controls.selectionMode = 5;
                }
                else if(SCollision::collide(mPos, controls.rotationPointCircle)) {
                    set_selected_items();
                    controls.selectionMode = 6;
                }
                else if(SCollision::collide(drawP.controls.currentMouseWorldPos, controls.finalSelectionAABB)) {
                    controls.moveStartAt = drawP.controls.currentMouseWorldPos;
                    set_selected_items();
                    controls.selectionMode = 3;
                }
                else {
                   controls.selectionMode = 0;
                   drawP.reset_selection();
                }
            }
            else if(deleteHit || cutHit) {
                selection_to_clipboard();
                std::vector<std::pair<uint64_t, std::shared_ptr<DrawComponent>>> erasedCompVal; // <placement order, object>
                drawP.components.client_erase_if([&](uint64_t oldPlacement, const auto& c) {
                    if(c->obj->selected) {
                        erasedCompVal.emplace_back(std::pair<uint64_t, std::shared_ptr<DrawComponent>>{oldPlacement, c->obj});
                        DrawComponent::client_send_erase(drawP, c->id);
                        return true;
                    }
                    return false;
                });
                drawP.world.undo.push(UndoManager::UndoRedoPair{
                    [&, erasedCompVal]() {
                        for(auto& comp : erasedCompVal)
                            if(drawP.components.get_id(comp.second) != ServerClientID{0, 0})
                                return false;

                        for(auto& comp : erasedCompVal | std::views::reverse) {
                            drawP.components.client_insert(comp.first, comp.second);
                            comp.second->client_send_place(drawP, comp.first);
                        }
                        drawP.reset_tools();
                        return true;
                    },
                    [&, erasedCompVal]() {
                        for(auto& comp : erasedCompVal)
                            if(drawP.components.get_id(comp.second) == ServerClientID{0, 0})
                                return false;

                        for(auto& comp : erasedCompVal) {
                            ServerClientID compID;
                            drawP.components.client_erase(comp.second, compID);
                            DrawComponent::client_send_erase(drawP, compID);
                        }
                        drawP.reset_tools();
                        return true;
                    }
                });
                controls.selectionMode = 0;
            }
            else if(copyHit)
                selection_to_clipboard();
            else if(unselectHit) {
                drawP.reset_selection();
                controls.selectionMode = 0;
            }
            break;
        }
        case 3: {
            WorldVec translationVec = drawP.controls.currentMouseWorldPos - controls.moveStartAt;
            if(drawP.controls.leftClickHeld) {
                for(auto& item : controls.selectedItems) {
                    auto newCoords = item.oldCoords;
                    newCoords.translate(translationVec);
                    item.comp->coords = newCoords;
                    item.comp->updateDraw = true;
                    item.comp->client_send_transform_temp(drawP, item.id);
                    item.comp->lastTransformTime = std::chrono::steady_clock::now();
                }
            }
            else {
                commit_transformation();
                controls.selectionMode = create_selection_aabb() ? 2 : 0;
                controls.rotationCenter = controls.finalSelectionAABB.center();
            }
            break;
        }
        case 4: {
            WorldVec centerToScaleStart = controls.scaleStartAt - controls.scaleCenterPoint;
            Vector2f centerToScaleStartInCamSpace = drawP.world.drawData.cam.c.normalized_dir_to_space(centerToScaleStart.normalized());
            Vector2f scaleCenterPointInCamSpace = drawP.world.drawData.cam.c.to_space(controls.scaleCenterPoint);
            controls.scaleCurrentPoint = drawP.world.drawData.cam.c.from_space(project_point_on_line(drawP.world.main.input.mouse.pos, scaleCenterPointInCamSpace, (scaleCenterPointInCamSpace + centerToScaleStartInCamSpace).eval()));

            WorldVec centerToScaleCurrent = controls.scaleCurrentPoint - controls.scaleCenterPoint;

            WorldScalar centerToScaleStartNorm = centerToScaleStart.norm();
            WorldScalar centerToScaleCurrentNorm = centerToScaleCurrent.norm();
            bool isAnyNumberZero = centerToScaleCurrentNorm == WorldScalar(0) || centerToScaleStartNorm == WorldScalar(0);
            if(!isAnyNumberZero) {
                bool flipScale = centerToScaleStartNorm < centerToScaleCurrentNorm;
                WorldScalar scaleAmount;
                if(flipScale)
                    scaleAmount = centerToScaleCurrentNorm / centerToScaleStartNorm;
                else
                    scaleAmount = centerToScaleStartNorm / centerToScaleCurrentNorm;

                if(drawP.controls.leftClickHeld) {
                    for(auto& item : controls.selectedItems) {
                        auto newCoords = item.oldCoords;
                        newCoords.scale_about(controls.scaleCenterPoint, scaleAmount, flipScale);
                        item.comp->coords = newCoords;
                        item.comp->client_send_transform_temp(drawP, item.id);
                        item.comp->updateDraw = true;
                        item.comp->lastTransformTime = std::chrono::steady_clock::now();
                    }
                }
                else {
                    commit_transformation();
                    controls.selectionMode = create_selection_aabb() ? 2 : 0;
                    controls.rotationCenter = controls.finalSelectionAABB.center();
                }
            }
            break;
        }
        case 5: {
            controls.rotationCenter = drawP.controls.currentMouseWorldPos;
            if(!drawP.controls.leftClickHeld) {
                controls.selectionMode = 2;
            }
            break;
        }
        case 6: {
            Vector2f rotationPointDiff = drawP.world.main.input.mouse.pos - drawP.world.drawData.cam.c.to_space(controls.rotationCenter);
            controls.rotationPointAngle = std::atan2(rotationPointDiff.y(), rotationPointDiff.x());
            if(drawP.controls.leftClickHeld) {
                for(auto& item : controls.selectedItems) {
                    auto newCoords = item.oldCoords;
                    newCoords.rotate_about(controls.rotationCenter, controls.rotationPointAngle);
                    item.comp->coords = newCoords;
                    item.comp->updateDraw = true;
                    item.comp->client_send_transform_temp(drawP, item.id);
                    item.comp->lastTransformTime = std::chrono::steady_clock::now();
                }
            }
            else {
                commit_transformation();
                controls.rotationPointAngle = 0.0;
                controls.selectionMode = create_selection_aabb() ? 2 : 0;
            }
            break;
        }
    }

    update_drag_circles();
}

void RectSelectTool::paste_clipboard() {
    auto& clipboard = drawP.world.main.clipboard;
    if(!clipboard.components.empty()) {
        drawP.reset_tools();
        drawP.controls.selectedTool = DrawingProgram::TOOL_RECTSELECT;
        drawP.controls.previousSelected = DrawingProgram::TOOL_RECTSELECT;
        uint64_t allPlacement = drawP.components.client_list().size();
        std::vector<std::shared_ptr<DrawComponent>> placedComponents;
        WorldVec mousePos = drawP.world.get_mouse_world_pos();
        WorldVec moveVec = clipboard.pos - mousePos;
        std::unordered_map<ServerClientID, ServerClientID> resourceRemapIDs;
        for(auto& r : clipboard.resources)
            resourceRemapIDs[r.first] = drawP.world.rMan.add_resource(r.second);
        for(auto& c : clipboard.components) {
            uint64_t placement = drawP.components.client_list().size();
            std::shared_ptr<DrawComponent> newC = c->copy();
            newC->remap_resource_ids(resourceRemapIDs);
            newC->coords.translate(-moveVec);
            newC->selected = true;

            drawP.components.client_insert(placement, newC);
            newC->final_update(drawP);
            newC->client_send_place(drawP, placement);
            placedComponents.emplace_back(newC);
        }
        set_selected_items();
        create_selection_aabb();
        controls.rotationCenter = controls.finalSelectionAABB.center();
        controls.selectionMode = 2;
        drawP.add_undo_place_components(allPlacement, placedComponents);
    }
}

void RectSelectTool::selection_to_clipboard() {
    auto& clipboard = drawP.world.main.clipboard;
    clipboard.components.clear();
    std::unordered_set<ServerClientID> resourceSet;
    for(auto& c : drawP.components.client_list()) {
        if(c->obj->selected) {
            c->obj->get_used_resources(resourceSet);
            clipboard.components.emplace_back(c->obj->copy());
        }
    }
    drawP.world.rMan.copy_resource_set_to_map(resourceSet, clipboard.resources);
    clipboard.pos = controls.finalSelectionAABB.center();
}

Vector2f RectSelectTool::get_rotation_point_pos_from_angle(double angle) {
    return Vector2f{drawP.world.drawData.cam.c.to_space(controls.rotationCenter) + ROTATION_POINTS_DISTANCE * Vector2f{std::cos(angle), std::sin(angle)}};
}

void RectSelectTool::update_drag_circles() {
    controls.scaleSelectionCircle = SCollision::Circle<float>(drawP.world.drawData.cam.c.to_space(controls.scaleCurrentPoint), DRAG_POINT_RADIUS);
    controls.rotationCenterCircle = SCollision::Circle<float>(drawP.world.drawData.cam.c.to_space(controls.rotationCenter), DRAG_POINT_RADIUS);
    controls.rotationPoint = get_rotation_point_pos_from_angle(controls.rotationPointAngle);
    controls.rotationPointCircle = SCollision::Circle<float>(controls.rotationPoint, ROTATION_POINT_RADIUS_MULTIPLIER * DRAG_POINT_RADIUS);
}

bool RectSelectTool::create_selection_aabb() {
    controls.coords = drawP.world.drawData.cam.c;

    controls.finalSelectionAABB = SCollision::AABB<WorldScalar>();
    bool first = true;
    for(auto& c : drawP.components.client_list()) {
        if(c->obj->selected && c->obj->get_world_bounds()) {
            if(first) {
                controls.finalSelectionAABB = *c->obj->get_world_bounds();
                first = false;
            }
            else
                controls.finalSelectionAABB.include_aabb_in_bounds(*c->obj->get_world_bounds());
        }
    }

    controls.scaleCurrentPoint = controls.finalSelectionAABB.max;

    return !first;
}

void RectSelectTool::set_selected_items() {
    controls.selectedItems.clear();
    for(auto& c : drawP.components.client_list()) {
        if(c->obj->selected) {
            controls.selectedItems.emplace_back(c->obj, c->id, c->obj->coords);
            c->obj->lastTransformTime = std::chrono::steady_clock::now();
        }
    }
}

void RectSelectTool::commit_transformation() {
    std::vector<CoordSpaceHelper> finalCoords;
    for(auto& item : controls.selectedItems) {
        item.comp->client_send_transform_final(drawP, item.id);
        item.comp->finalize_update(drawP.colliderAllocated);
        finalCoords.emplace_back(item.comp->coords);
    }

    auto sItems = controls.selectedItems;

    drawP.world.undo.push(UndoManager::UndoRedoPair{
        [&, sItems, finalCoords]() {
            for(auto& item : sItems) {
                item.comp->coords = item.oldCoords;
                item.comp->client_send_transform_final(drawP, drawP.components.get_id(item.comp));
                item.comp->finalize_update(drawP.colliderAllocated);
            }
            drawP.reset_tools();
            return true;
        },
        [&, sItems, finalCoords]() {
            for(auto [item, finalCoord]  : std::views::zip(sItems, finalCoords)) {
                item.comp->coords = finalCoord;
                item.comp->client_send_transform_final(drawP, drawP.components.get_id(item.comp));
                item.comp->finalize_update(drawP.colliderAllocated);
            }
            drawP.reset_tools();
            return true;
        }
    });
}

void RectSelectTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(controls.selectionMode != 0) {
        canvas->save();
        controls.coords.transform_sk_canvas(canvas, drawData);

        SkPaint selectionPaint;
        selectionPaint.setColor4f({0.3f, 0.6f, 0.9f, 0.4f});

        if(controls.selectionMode == 1) {
            SkRect r = SkRect::MakeLTRB(controls.newT[0].x(), controls.newT[0].y(), controls.newT[2].x(), controls.newT[2].y());
            canvas->drawRect(r, selectionPaint);
            canvas->restore();
        }
        if(controls.selectionMode == 2 || controls.selectionMode == 5) {
            auto triArray = triangle_from_rect_points(controls.finalSelectionAABB.min, controls.finalSelectionAABB.max);
            std::array<Vector2f, 4> triArray2;
            triArray2[0] = controls.coords.to_space(controls.finalSelectionAABB.min);
            triArray2[1] = controls.coords.to_space(controls.finalSelectionAABB.top_right());
            triArray2[2] = controls.coords.to_space(controls.finalSelectionAABB.bottom_left());
            triArray2[3] = controls.coords.to_space(controls.finalSelectionAABB.max);
            SkPath a;
            a.moveTo(triArray2[0].x(), triArray2[0].y());
            a.lineTo(triArray2[1].x(), triArray2[1].y());
            a.lineTo(triArray2[2].x(), triArray2[2].y());
            a.close();
            a.moveTo(triArray2[1].x(), triArray2[1].y());
            a.lineTo(triArray2[2].x(), triArray2[2].y());
            a.lineTo(triArray2[3].x(), triArray2[3].y());
            a.close();
            canvas->drawPath(a, selectionPaint);
            // SkPaint rotateCirclePaint(SkColor4f{0.9f, 0.5f, 0.1f, 1.0f});
        }

        canvas->restore();

        if(controls.selectionMode == 6 || controls.selectionMode == 5 || controls.selectionMode == 2) {
            drawP.draw_drag_circle(canvas, controls.rotationPoint, {0.9f, 0.5f, 0.1f, 1.0f}, drawData, ROTATION_POINT_RADIUS_MULTIPLIER);
            drawP.draw_drag_circle(canvas, drawData.cam.c.to_space(controls.rotationCenter), {0.9f, 0.5f, 0.1f, 1.0f}, drawData);
        }
        if(controls.selectionMode == 4 || controls.selectionMode == 2) {
            drawP.draw_drag_circle(canvas, drawData.cam.c.to_space(controls.scaleCurrentPoint), {0.1f, 0.9f, 0.9f, 1.0f}, drawData);
            if(controls.selectionMode == 4)
                drawP.draw_drag_circle(canvas, drawData.cam.c.to_space(controls.scaleCenterPoint), {0.0f, 0.0f, 0.0f, 0.0f}, drawData);
        }
    }
}
