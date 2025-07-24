#include "DrawingProgramSelection.hpp"
#include "DrawingProgram.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include <Helpers/MathExtras.hpp>
#include <Helpers/SCollision.hpp>
#include <Helpers/Parallel.hpp>

DrawingProgramSelection::DrawingProgramSelection(DrawingProgram& initDrawP):
    cache(initDrawP),
    drawP(initDrawP)
{}

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
        });
        selectedSet.clear();
        cache.clear();
        selectionTransformCoords = CoordSpaceHelper();
        drawP.compCache.force_rebuild(drawP.components.client_list());
    }
}

void DrawingProgramSelection::update() {
    if(is_something_selected()) {
        if(cache.get_unsorted_component_list().size() >= 1000 && (std::chrono::steady_clock::now() - cache.get_last_bvh_build_time()) >= std::chrono::seconds(5))
            cache.force_rebuild(std::vector<CollabListType::ObjectInfoPtr>(selectedSet.begin(), selectedSet.end()));
        if(drawP.world.main.input.key(InputManager::KEY_DRAW_UNSELECT).pressed)
            deselect_all();
        if(drawP.world.main.input.key(InputManager::KEY_TEXT_CTRL).held)
            selectionTransformCoords.scale_about_double(selectionTransformCoords.pos, 1.01);
        if(drawP.world.main.input.key(InputManager::KEY_TEXT_SHIFT).held)
            selectionTransformCoords.scale_about_double(selectionTransformCoords.pos, 0.99);

        selectionRectMin = selectionTransformCoords.from_space_world(initialSelectionAABB.min);
        selectionRectMax = selectionTransformCoords.from_space_world(initialSelectionAABB.max);

        scaleData.handlePoint = drawP.world.drawData.cam.c.to_space(selectionTransformCoords.from_space_world(initialSelectionAABB.max));

        rebuild_cam_space();

        switch(transformOpHappening) {
            case TransformOperation::NONE:
                if(drawP.controls.leftClick) {
                    if(mouse_collided_with_scale_point()) {
                        scaleData.currentPos = scaleData.startPos = drawP.world.get_mouse_world_pos();
                        scaleData.centerPos = selectionTransformCoords.from_space_world(initialSelectionAABB.center());
                        scaleData.startingSelectionTransformCoords = selectionTransformCoords;
                        transformOpHappening = TransformOperation::SCALE;
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
                else
                    transformOpHappening = TransformOperation::NONE;
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

                        selectionTransformCoords = scaleData.startingSelectionTransformCoords;
                        selectionTransformCoords.scale_about(scaleData.centerPos, scaleAmount, flipScale);
                    }
                }
                else
                    transformOpHappening = TransformOperation::NONE;
                break;
            case TransformOperation::ROTATE:
                break;
        }
    }
}

void DrawingProgramSelection::rebuild_cam_space() {
    std::array<WorldVec, 4> collideRectTriangleVerticesWorld = triangle_from_rect_points(selectionRectMin, selectionRectMax);
    SCollision::ColliderCollection<WorldScalar> collideRectWorld;
    collideRectWorld.triangle.emplace_back(collideRectTriangleVerticesWorld[0], collideRectTriangleVerticesWorld[1], collideRectTriangleVerticesWorld[2]);
    collideRectWorld.triangle.emplace_back(collideRectTriangleVerticesWorld[0], collideRectTriangleVerticesWorld[2], collideRectTriangleVerticesWorld[3]);
    collideRectWorld.recalculate_bounds();

    camSpaceSelection = drawP.world.drawData.cam.c.world_collider_to_coords<SCollision::ColliderCollection<float>>(collideRectWorld);
}

bool DrawingProgramSelection::mouse_collided_with_selection() {
    return mouse_collided_with_selection_aabb() || mouse_collided_with_scale_point();
}

bool DrawingProgramSelection::mouse_collided_with_selection_aabb() {
    return SCollision::collide(camSpaceSelection, drawP.world.main.input.mouse.pos);
}

bool DrawingProgramSelection::mouse_collided_with_scale_point() {
    return SCollision::collide(SCollision::Circle(scaleData.handlePoint, DRAG_POINT_RADIUS), drawP.world.main.input.mouse.pos);
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

        drawP.draw_drag_circle(canvas, scaleData.handlePoint, {0.1f, 0.9f, 0.9f, 1.0f}, drawData);
    }
}
