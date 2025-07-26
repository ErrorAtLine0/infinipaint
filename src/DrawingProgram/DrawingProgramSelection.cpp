#include "DrawingProgramSelection.hpp"
#include "DrawingProgram.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include <Helpers/MathExtras.hpp>
#include <Helpers/SCollision.hpp>
#include <Helpers/Parallel.hpp>

#define ROTATION_POINT_RADIUS_MULTIPLIER 0.7f
#define ROTATION_POINTS_DISTANCE 20.0f

DrawingProgramSelection::DrawingProgramSelection(DrawingProgram& initDrawP):
    cache(initDrawP),
    drawP(initDrawP)
{}

const std::unordered_set<CollabListType::ObjectInfoPtr>& DrawingProgramSelection::get_selected_set() {
    return selectedSet;
}

void DrawingProgramSelection::add_from_cam_coord_collider_to_selection(const SCollision::ColliderCollection<float>& cC) {
    auto cCWorld = drawP.world.drawData.cam.c.collider_to_world<SCollision::ColliderCollection<WorldScalar>, SCollision::ColliderCollection<float>>(cC);

    std::unordered_set<CollabListType::ObjectInfoPtr> selectedComponents;

    std::function<void(const std::shared_ptr<DrawingProgramCacheBVHNode>&)> fullyCollidedFunc = [&](const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode) {
        if(bvhNode) {
            for(auto& c : bvhNode->components)
                selectedComponents.emplace(c);
            for(auto& p : bvhNode->children)
                fullyCollidedFunc(p);
        }
    };
    drawP.compCache.traverse_bvh_erase_function(cCWorld.bounds, [&](const auto& bvhNode, auto& comps) {
        if(bvhNode &&
           SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.min)) &&
           SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.max)) &&
           SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.top_right())) &&
           SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.bottom_left()))) {
            fullyCollidedFunc(bvhNode);
            drawP.compCache.invalidate_cache_at_aabb_before_pos(bvhNode->bounds, 0);
            return true;
        }
        std::erase_if(comps, [&](auto& c) {
            if(c->obj->collides_with(drawP.world.drawData.cam.c, cCWorld, cC, drawP.colliderAllocated)) {
                selectedComponents.emplace(c);
                drawP.compCache.invalidate_cache_at_aabb_before_pos(c->obj->worldAABB.value(), c->obj->collabListInfo.lock()->pos);
                return true;
            }
            return false;
        });
        return false;
    });

    selectedSet = selectedComponents;
    for(auto& obj : selectedSet)
        cache.add_component(obj);
    calculate_aabb();
    calculate_initial_rotate_center_location();
}

bool DrawingProgramSelection::is_something_selected() {
    return !selectedSet.empty();
}

void DrawingProgramSelection::calculate_aabb() {
    if(is_something_selected()) {
        initialSelectionAABB = (*selectedSet.begin())->obj->worldAABB.value();
        for(auto& c : selectedSet)
            initialSelectionAABB.include_aabb_in_bounds(c->obj->worldAABB.value());
    }
}

void DrawingProgramSelection::deselect_all() {
    if(is_something_selected()) {
        std::vector<CollabListType::ObjectInfoPtr> a(selectedSet.begin(), selectedSet.end());
        parallel_loop_container(a, [&](auto& obj) {
            obj->obj->coords = selectionTransformCoords.other_coord_space_from_this_space(obj->obj->coords);
            obj->obj->final_update(drawP, false);
            obj->obj->client_send_transform(drawP);
        });
        selectedSet.clear();
        cache.clear();
        selectionTransformCoords = CoordSpaceHelper();
        drawP.force_rebuild_cache();
        transformOpHappening = TransformOperation::NONE;
    }
}

bool DrawingProgramSelection::is_selected(const CollabListType::ObjectInfoPtr& objToCheck) {
    return selectedSet.contains(objToCheck);
}

void DrawingProgramSelection::update() {
    if(is_something_selected()) {

        if(cache.check_if_rebuild_should_occur())
            cache.test_rebuild(std::vector<CollabListType::ObjectInfoPtr>(selectedSet.begin(), selectedSet.end()), true);

        if(drawP.world.main.input.key(InputManager::KEY_DRAW_UNSELECT).pressed)
            deselect_all();

        selectionRectPoints[0] = selectionTransformCoords.from_space_world(initialSelectionAABB.min);
        selectionRectPoints[1] = selectionTransformCoords.from_space_world(initialSelectionAABB.bottom_left());
        selectionRectPoints[2] = selectionTransformCoords.from_space_world(initialSelectionAABB.max);
        selectionRectPoints[3] = selectionTransformCoords.from_space_world(initialSelectionAABB.top_right());

        scaleData.handlePoint = drawP.world.drawData.cam.c.to_space(selectionTransformCoords.from_space_world(initialSelectionAABB.max));
        rotateData.centerHandlePoint = drawP.world.drawData.cam.c.to_space(selectionTransformCoords.from_space_world(rotateData.centerPos));
        rotateData.handlePoint = get_rotation_point_pos_from_angle(rotateData.rotationAngle);

        rebuild_cam_space();

        switch(transformOpHappening) {
            case TransformOperation::NONE:
                if(drawP.controls.leftClick) {
                    if(mouse_collided_with_scale_point()) {
                        scaleData.currentPos = scaleData.startPos = drawP.world.get_mouse_world_pos();
                        scaleData.centerPos = selectionTransformCoords.from_space_world(initialSelectionAABB.center());
                        startingSelectionTransformCoords = selectionTransformCoords;
                        transformOpHappening = TransformOperation::SCALE;
                    }
                    else if(mouse_collided_with_rotate_center_handle_point())
                        transformOpHappening = TransformOperation::ROTATE_RELOCATE_CENTER;
                    else if(mouse_collided_with_rotate_handle_point()) {
                        rotateData.rotationAngle = 0.0;
                        startingSelectionTransformCoords = selectionTransformCoords;
                        transformOpHappening = TransformOperation::ROTATE;
                    }
                    else if(mouse_collided_with_selection_aabb())
                        transformOpHappening = TransformOperation::TRANSLATE;
                    else
                        deselect_all();
                }
                break;
            case TransformOperation::TRANSLATE:
                if(drawP.controls.leftClickHeld)
                    selectionTransformCoords.translate(drawP.world.drawData.cam.c.dir_from_space(drawP.world.main.input.mouse.move));
                else {
                    transformOpHappening = TransformOperation::NONE;
                    calculate_initial_rotate_center_location();
                }
                break;
            case TransformOperation::SCALE:
                if(drawP.controls.leftClickHeld) {
                    WorldVec centerToScaleStart = scaleData.startPos - scaleData.centerPos;
                    Vector2f centerToScaleStartInCamSpace = drawP.world.drawData.cam.c.normalized_dir_to_space(centerToScaleStart.normalized());
                    Vector2f scaleCenterPointInCamSpace = drawP.world.drawData.cam.c.to_space(scaleData.centerPos);
                    scaleData.currentPos = drawP.world.drawData.cam.c.from_space(project_point_on_line(drawP.world.main.input.mouse.pos, scaleCenterPointInCamSpace, (scaleCenterPointInCamSpace + centerToScaleStartInCamSpace).eval()));

                    WorldVec centerToScaleCurrent = scaleData.currentPos - scaleData.centerPos;

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

                        selectionTransformCoords = startingSelectionTransformCoords;
                        selectionTransformCoords.scale_about(scaleData.centerPos, scaleAmount, flipScale);
                    }
                }
                else {
                    transformOpHappening = TransformOperation::NONE;
                    calculate_initial_rotate_center_location();
                }
                break;
            case TransformOperation::ROTATE_RELOCATE_CENTER:
                if(drawP.controls.leftClickHeld)
                    rotateData.centerPos = selectionTransformCoords.to_space_world(drawP.world.get_mouse_world_pos());
                else
                    transformOpHappening = TransformOperation::NONE;
                break;
            case TransformOperation::ROTATE:
                if(drawP.controls.leftClickHeld) {
                    Vector2f rotationPointDiff = drawP.world.main.input.mouse.pos - rotateData.centerHandlePoint;
                    rotateData.rotationAngle = std::atan2(rotationPointDiff.y(), rotationPointDiff.x());
                    selectionTransformCoords = startingSelectionTransformCoords;
                    selectionTransformCoords.rotate_about(selectionTransformCoords.from_space_world(rotateData.centerPos), rotateData.rotationAngle);
                }
                else
                    transformOpHappening = TransformOperation::NONE;
                break;
        }
    }
}

void DrawingProgramSelection::rebuild_cam_space() {
    SCollision::ColliderCollection<WorldScalar> collideRectWorld;
    collideRectWorld.triangle.emplace_back(selectionRectPoints[0], selectionRectPoints[1], selectionRectPoints[2]);
    collideRectWorld.triangle.emplace_back(selectionRectPoints[0], selectionRectPoints[2], selectionRectPoints[3]);
    collideRectWorld.recalculate_bounds();

    camSpaceSelection = drawP.world.drawData.cam.c.world_collider_to_coords<SCollision::ColliderCollection<float>>(collideRectWorld);
}

void DrawingProgramSelection::calculate_initial_rotate_center_location() {
    rotateData.centerPos = initialSelectionAABB.center();
}

bool DrawingProgramSelection::mouse_collided_with_selection() {
    return mouse_collided_with_selection_aabb() || mouse_collided_with_scale_point() || mouse_collided_with_rotate_center_handle_point() || mouse_collided_with_rotate_handle_point();
}

bool DrawingProgramSelection::mouse_collided_with_selection_aabb() {
    return SCollision::collide(camSpaceSelection, drawP.world.main.input.mouse.pos);
}

bool DrawingProgramSelection::mouse_collided_with_scale_point() {
    return SCollision::collide(SCollision::Circle(scaleData.handlePoint, DRAG_POINT_RADIUS), drawP.world.main.input.mouse.pos);
}

bool DrawingProgramSelection::mouse_collided_with_rotate_center_handle_point() {
    return SCollision::collide(SCollision::Circle(rotateData.centerHandlePoint, DRAG_POINT_RADIUS), drawP.world.main.input.mouse.pos);
}

bool DrawingProgramSelection::mouse_collided_with_rotate_handle_point() {
    return SCollision::collide(SCollision::Circle(rotateData.handlePoint, DRAG_POINT_RADIUS * ROTATION_POINT_RADIUS_MULTIPLIER), drawP.world.main.input.mouse.pos);
}

Vector2f DrawingProgramSelection::get_rotation_point_pos_from_angle(double angle) {
    return Vector2f{rotateData.centerHandlePoint + ROTATION_POINTS_DISTANCE * Vector2f{std::cos(angle), std::sin(angle)}};
}

void DrawingProgramSelection::draw_components(SkCanvas* canvas, const DrawData& drawData) {
    if(is_something_selected()) {
        DrawData selectionDrawData = drawData;
        selectionDrawData.cam.c = selectionTransformCoords.other_coord_space_to_this_space(selectionDrawData.cam.c);
        selectionDrawData.cam.set_viewing_area(drawP.world.main.window.size.cast<float>());
        selectionDrawData.refresh_draw_optimizing_values();

        cache.refresh_all_draw_cache(selectionDrawData);
        cache.draw_components_to_canvas(canvas, selectionDrawData);
    }
}

void DrawingProgramSelection::draw_gui(SkCanvas* canvas, const DrawData& drawData) {
    if(is_something_selected() && !camSpaceSelection.triangle.empty()) {
        SkPath selectionRectPath;
        selectionRectPath.moveTo(convert_vec2<SkPoint>(camSpaceSelection.triangle[0].p[0]));
        selectionRectPath.lineTo(convert_vec2<SkPoint>(camSpaceSelection.triangle[0].p[1]));
        selectionRectPath.lineTo(convert_vec2<SkPoint>(camSpaceSelection.triangle[0].p[2]));
        selectionRectPath.lineTo(convert_vec2<SkPoint>(camSpaceSelection.triangle[1].p[2]));
        selectionRectPath.close();
        SkPaint p{SkColor4f{0.3f, 0.6f, 0.9f, 0.4f}};
        canvas->drawPath(selectionRectPath, p);

        if(transformOpHappening == TransformOperation::NONE || transformOpHappening == TransformOperation::ROTATE || transformOpHappening == TransformOperation::ROTATE_RELOCATE_CENTER)
            drawP.draw_drag_circle(canvas, rotateData.centerHandlePoint, {0.9f, 0.5f, 0.1f, 1.0f}, drawData);
        if(transformOpHappening == TransformOperation::NONE || transformOpHappening == TransformOperation::ROTATE)
            drawP.draw_drag_circle(canvas, rotateData.handlePoint, {0.9f, 0.5f, 0.1f, 1.0f}, drawData, ROTATION_POINT_RADIUS_MULTIPLIER);
        if(transformOpHappening == TransformOperation::NONE || transformOpHappening == TransformOperation::SCALE)
            drawP.draw_drag_circle(canvas, scaleData.handlePoint, {0.1f, 0.9f, 0.9f, 1.0f}, drawData);
    }
}
