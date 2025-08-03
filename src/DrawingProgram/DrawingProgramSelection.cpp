#include "DrawingProgramSelection.hpp"
#include "DrawingProgram.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include <Helpers/MathExtras.hpp>
#include <Helpers/SCollision.hpp>
#include <Helpers/Parallel.hpp>

#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include <include/gpu/graphite/Surface.h>
#elif USE_SKIA_BACKEND_GANESH
    #include <include/gpu/ganesh/GrDirectContext.h>
    #include <include/gpu/ganesh/SkSurfaceGanesh.h>
#endif

#include <include/effects/SkImageFilters.h>

#define ROTATION_POINT_RADIUS_MULTIPLIER 0.7f
#define ROTATION_POINTS_DISTANCE 20.0f

DrawingProgramSelection::DrawingProgramSelection(DrawingProgram& initDrawP):
    cache(initDrawP),
    drawP(initDrawP)
{}

const std::unordered_set<CollabListType::ObjectInfoPtr>& DrawingProgramSelection::get_selected_set() {
    return selectedSet;
}

void DrawingProgramSelection::erase_component(const CollabListType::ObjectInfoPtr& objToCheck) {
    if(is_selected(objToCheck)) {
        cache.erase_component(objToCheck);
        selectedSet.erase(objToCheck);
    }
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
        if(bvhNode && (bvhNode->coords.inverseScale << 9) < drawP.world.drawData.cam.c.inverseScale &&
           SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.min)) &&
           SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.max)) &&
           SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.top_right())) &&
           SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.bottom_left()))) {
            fullyCollidedFunc(bvhNode);
            drawP.compCache.invalidate_cache_at_aabb_before_pos(bvhNode->bounds, 0);
            return true;
        }
        std::erase_if(comps, [&](auto& c) {
            if(c->obj->collides_with(drawP.world.drawData.cam.c, cCWorld, cC)) {
                selectedComponents.emplace(c);
                drawP.compCache.invalidate_cache_at_aabb_before_pos(c->obj->worldAABB.value(), c->obj->collabListInfo.lock()->pos);
                return true;
            }
            return false;
        });
        return false;
    });

    set_to_selection(selectedComponents);
}

void DrawingProgramSelection::set_to_selection(const std::unordered_set<CollabListType::ObjectInfoPtr>& newSelection) {
    selectedSet = newSelection;
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
        std::vector<std::pair<CollabListType::ObjectInfoPtr, CoordSpaceHelper>> transformsFrom;
        for(auto& transformedObj : a)
            transformsFrom.emplace_back(transformedObj, transformedObj->obj->coords);

        reset_selection_data();

        bool isSingleThread = a.size() < DrawingProgramCache::MINIMUM_COMPONENTS_TO_START_REBUILD;
        parallel_loop_container(a, [&](auto& obj) {
            obj->obj->coords = selectionTransformCoords.other_coord_space_from_this_space(obj->obj->coords);
            obj->obj->commit_transform(drawP, false);
        }, isSingleThread);
        if(!isSingleThread)
            drawP.force_rebuild_cache();
        else {
            for(auto& obj : a)
                drawP.compCache.add_component(obj);
        }

        reset_transform_data();

        std::vector<std::pair<CollabListType::ObjectInfoPtr, CoordSpaceHelper>> transformsTo;
        std::vector<std::pair<ServerClientID, CoordSpaceHelper>> transformsToSend;
        for(auto& transformedObj : a) {
            transformsToSend.emplace_back(transformedObj->id, transformedObj->obj->coords);
            transformsTo.emplace_back(transformedObj, transformedObj->obj->coords);
        }
        DrawComponent::client_send_transform_many(drawP, transformsToSend);

        drawP.world.undo.push(UndoManager::UndoRedoPair{
            [&, transformsFrom = transformsFrom]() {
                std::vector<std::pair<ServerClientID, CoordSpaceHelper>> transformsToSend;
                for(auto& [comp, coords] : transformsFrom) {
                    auto lockObjInfo = comp->obj->collabListInfo.lock();
                    if(lockObjInfo)
                        transformsToSend.emplace_back(lockObjInfo->id, coords);
                    else
                        return false;
                }

                bool isSingleThread = a.size() < DrawingProgramCache::MINIMUM_COMPONENTS_TO_START_REBUILD;
                parallel_loop_container(transformsFrom, [&](auto& p) {
                    auto& [comp, coords] = p;
                    comp->obj->coords = coords;
                    comp->obj->commit_transform(drawP, isSingleThread);
                }, isSingleThread);

                DrawComponent::client_send_transform_many(drawP, transformsToSend);

                if(!isSingleThread)
                    drawP.force_rebuild_cache();

                return true;
            },
            [&, transformsTo = transformsTo]() {
                std::vector<std::pair<ServerClientID, CoordSpaceHelper>> transformsToSend;
                for(auto& [comp, coords] : transformsTo) {
                    auto lockObjInfo = comp->obj->collabListInfo.lock();
                    if(lockObjInfo)
                        transformsToSend.emplace_back(lockObjInfo->id, coords);
                    else
                        return false;
                }

                bool isSingleThread = a.size() < DrawingProgramCache::MINIMUM_COMPONENTS_TO_START_REBUILD;
                parallel_loop_container(transformsTo, [&](auto& p) {
                    auto& [comp, coords] = p;
                    comp->obj->coords = coords;
                    comp->obj->commit_transform(drawP, isSingleThread);
                }, isSingleThread);

                DrawComponent::client_send_transform_many(drawP, transformsToSend);

                if(!isSingleThread)
                    drawP.force_rebuild_cache();

                return true;
            }
        });
    }
}

void DrawingProgramSelection::invalidate_cache_at_optional_aabb_before_pos(const std::optional<SCollision::AABB<WorldScalar>>& aabb, uint64_t placementToInvalidateAt) {
    cache.invalidate_cache_at_optional_aabb_before_pos(aabb, placementToInvalidateAt);
}

void DrawingProgramSelection::preupdate_component(const CollabListType::ObjectInfoPtr& objToCheck) {
    cache.preupdate_component(objToCheck);
}

bool DrawingProgramSelection::is_selected(const CollabListType::ObjectInfoPtr& objToCheck) {
    return selectedSet.contains(objToCheck);
}

void DrawingProgramSelection::delete_all() {
    if(is_something_selected()) {
        drawP.client_erase_set(selectedSet);
        reset_selection_data();
        reset_transform_data();
    }
}

// NOTE: Only run this if no transformations happened
void DrawingProgramSelection::selection_to_clipboard() {
    auto& clipboard = drawP.world.main.clipboard;
    std::unordered_set<ServerClientID> resourceSet;
    for(auto& c : selectedSet)
        c->obj->get_used_resources(resourceSet);
    std::vector<CollabListType::ObjectInfoPtr> compVecSort(selectedSet.begin(), selectedSet.end());
    std::sort(compVecSort.begin(), compVecSort.end(), [&](auto& a, auto& b) {
        return a->pos < b->pos;
    });
    clipboard.components.clear();
    for(auto& c : compVecSort)
        clipboard.components.emplace_back(c->obj);
    clipboard.pos = initialSelectionAABB.center();
}

void DrawingProgramSelection::update() {
    if(is_something_selected()) {
        if(cache.check_if_rebuild_should_occur())
            cache.test_rebuild(std::vector<CollabListType::ObjectInfoPtr>(selectedSet.begin(), selectedSet.end()), true);

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
                        scaleData.currentPos = scaleData.startPos = selectionTransformCoords.from_space_world(initialSelectionAABB.max);
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
                    else if(mouse_collided_with_selection_aabb()) {
                        translateData.startPos = drawP.world.get_mouse_world_pos();
                        startingSelectionTransformCoords = selectionTransformCoords;
                        transformOpHappening = TransformOperation::TRANSLATE;
                    }
                    else
                        deselect_all();
                }
                break;
            case TransformOperation::TRANSLATE:
                if(drawP.controls.leftClickHeld) {
                    selectionTransformCoords = startingSelectionTransformCoords;
                    selectionTransformCoords.translate(drawP.world.get_mouse_world_pos() - translateData.startPos);
                }
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

        if(drawP.world.main.input.key(InputManager::KEY_DRAW_UNSELECT).pressed)
            deselect_all();
        else if(drawP.world.main.input.key(InputManager::KEY_DRAW_DELETE).pressed)
            delete_all();
        else if(drawP.world.main.input.key(InputManager::KEY_COPY).pressed && startingSelectionTransformCoords == CoordSpaceHelper())
            selection_to_clipboard();
        else if(drawP.world.main.input.key(InputManager::KEY_CUT).pressed && startingSelectionTransformCoords == CoordSpaceHelper()) {
            selection_to_clipboard();
            delete_all();
        }
        else if(drawP.world.main.input.key(InputManager::KEY_PASTE).pressed) {
            deselect_all();
            paste_clipboard();
        }
    }
    else if(drawP.world.main.input.key(InputManager::KEY_PASTE).pressed && !drawP.prevent_undo_or_redo())
        paste_clipboard();
}

void DrawingProgramSelection::paste_clipboard() {
    auto& clipboard = drawP.world.main.clipboard;

    drawP.switch_to_tool(DrawingProgramToolType::RECTSELECT);

    uint64_t allPlacement = drawP.components.client_list().size();
    std::vector<std::shared_ptr<DrawComponent>> placedComponents;
    WorldVec mousePos = drawP.world.get_mouse_world_pos();
    WorldVec moveVec = drawP.world.main.clipboard.pos - mousePos;
    std::unordered_map<ServerClientID, ServerClientID> resourceRemapIDs;
    for(auto& r : clipboard.resources)
        resourceRemapIDs[r.first] = drawP.world.rMan.add_resource(r.second);
    for(auto& c : clipboard.components)
        placedComponents.emplace_back(c->deep_copy());
    drawP.addToCompCacheOnInsert = false;
    auto compListInserted = drawP.components.client_insert_ordered_vector_items(allPlacement, placedComponents);
    parallel_loop_container(compListInserted, [&](auto& c) {
        c->obj->remap_resource_ids(resourceRemapIDs);
        c->obj->coords.translate(-moveVec);
        c->obj->commit_transform(drawP, false);
    });
    DrawComponent::client_send_place_many(drawP, compListInserted);
    drawP.addToCompCacheOnInsert = true;
    std::unordered_set<CollabListType::ObjectInfoPtr> compSetInserted(compListInserted.begin(), compListInserted.end());
    set_to_selection(compSetInserted);
    drawP.add_undo_place_components(compSetInserted);
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

        SkImageInfo imgInfo = canvas->imageInfo();
        sk_sp<SkSurface> surface;
        #ifdef USE_SKIA_BACKEND_GRAPHITE
            surface = SkSurfaces::RenderTarget(drawP.world.main.window.recorder(), imgInfo, skgpu::Mipmapped::kNo, drawP.world.main.window.defaultMSAASurfaceProps);
        #elif USE_SKIA_BACKEND_GANESH
            surface = SkSurfaces::RenderTarget(drawP.world.main.window.ctx.get(), skgpu::Budgeted::kNo, imgInfo, drawP.world.main.window.defaultMSAASampleCount, &drawP.world.main.window.defaultMSAASurfaceProps);
        #endif
        if(!surface)
            throw std::runtime_error("[DrawingProgramSelection::draw_components] Could not make temporary surface");

        cache.draw_components_to_canvas(surface->getCanvas(), selectionDrawData);

        SkPaint glowBlurP;
        glowBlurP.setImageFilter(SkImageFilters::Blur(5, 5, nullptr));

        canvas->drawImage(surface->makeTemporaryImage(), 0, 0, SkSamplingOptions{SkFilterMode::kLinear}, &glowBlurP);
        canvas->drawImage(surface->makeTemporaryImage(), 0, 0, SkSamplingOptions{SkFilterMode::kLinear}, nullptr);
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

void DrawingProgramSelection::reset_selection_data() {
    selectedSet.clear();
    cache.clear();
}

void DrawingProgramSelection::reset_transform_data() {
    selectionTransformCoords = CoordSpaceHelper();
    transformOpHappening = TransformOperation::NONE;
    translateData = TranslationData();
    scaleData = ScaleData();
    rotateData = RotationData();
}
