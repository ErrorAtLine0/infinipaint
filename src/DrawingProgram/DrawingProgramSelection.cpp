#include "DrawingProgramSelection.hpp"
#include "DrawingProgram.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include "../InputManager.hpp"
#include <Helpers/Parallel.hpp>
#include <Helpers/Logger.hpp>

#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include <include/gpu/graphite/Surface.h>
#elif USE_SKIA_BACKEND_GANESH
    #include <include/gpu/ganesh/GrDirectContext.h>
    #include <include/gpu/ganesh/SkSurfaceGanesh.h>
#endif
#include <include/effects/SkImageFilters.h>
#include <include/core/SkPathBuilder.h>

#define ROTATION_POINT_RADIUS_MULTIPLIER 0.7f
#define ROTATION_POINTS_DISTANCE 20.0f

DrawingProgramSelection::DrawingProgramSelection(DrawingProgram& initDrawP):
    drawP(initDrawP)
{}

void DrawingProgramSelection::add_from_cam_coord_collider_to_selection(const SCollision::ColliderCollection<float>& cC, DrawingProgramLayerManager::LayerSelector layerSelector, bool frontObjectOnly) {
    std::vector<CanvasComponentContainer::ObjInfo*> selectedComponents;
    if(frontObjectOnly) {
        CanvasComponentContainer::ObjInfo* a = drawP.drawCache.get_front_object_colliding_with_in_editing_layer(cC);
        if(a) {
            selectedComponents.emplace_back(a);
            drawP.drawCache.erase_component(a);
        }
    }
    else {
        auto cCWorld = drawP.world.drawData.cam.c.collider_to_world<SCollision::ColliderCollection<WorldScalar>, SCollision::ColliderCollection<float>>(cC);
        drawP.drawCache.traverse_bvh_run_function(cCWorld.bounds, erase_select_objects_in_bvh_func(selectedComponents, cC, cCWorld, layerSelector));
    }
    add_to_selection(selectedComponents);
}

void DrawingProgramSelection::remove_from_cam_coord_collider_to_selection(const SCollision::ColliderCollection<float>& cC, DrawingProgramLayerManager::LayerSelector layerSelector, bool frontObjectOnly) {
    if(frontObjectOnly) {
        CanvasComponentContainer::ObjInfo* p = get_front_object_colliding_with_in_editing_layer(cC);
        if(p) {
            drawP.drawCache.add_component(p);
            std::erase(selectedSet, p);
        }
    }
    else {
        auto cCWorld = drawP.world.drawData.cam.c.collider_to_world<SCollision::ColliderCollection<WorldScalar>, SCollision::ColliderCollection<float>>(cC);
        std::erase_if(selectedSet, [&](auto& c) {
            if(drawP.layerMan.component_passes_layer_selector(c, layerSelector) && c->obj->collides_with(drawP.world.drawData.cam.c, cCWorld, cC)) {
                drawP.drawCache.add_component(c);
                return true;
            }
            return false;
        });
    }

    calculate_aabb();
}

std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>&)> DrawingProgramSelection::erase_select_objects_in_bvh_func(std::vector<CanvasComponentContainer::ObjInfo*>& selectedComponents, const SCollision::ColliderCollection<float>& cC, const SCollision::ColliderCollection<WorldScalar>& cCWorld, DrawingProgramLayerManager::LayerSelector layerSelector) {
    auto toRet = [&](const auto& bvhNode) {
        if(bvhNode && (bvhNode->coords.inverseScale << 9) < drawP.world.drawData.cam.c.inverseScale &&
           SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.min)) &&
           SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.max)) &&
           SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.top_right())) &&
           SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.bottom_left()))) {
            drawP.drawCache.invalidate_cache_at_aabb(bvhNode->bounds);
            drawP.drawCache.traverse_bvh_run_function_starting_at_node_no_collision_check(bvhNode, [&](const auto& bvhNodeChild) {
                drawP.drawCache.node_loop_erase_if_components(bvhNodeChild, [&](auto c) {
                    if(drawP.layerMan.component_passes_layer_selector(c, drawP.controls.layerSelector)) {
                        selectedComponents.emplace_back(c);
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
                selectedComponents.emplace_back(c);
                drawP.drawCache.invalidate_cache_at_aabb(c->obj->get_world_bounds());
                return true;
            }
            return false;
        });
        return true;
    };

    return toRet;
}

void DrawingProgramSelection::add_to_selection(const std::vector<CanvasComponentContainer::ObjInfo*>& newSelection) {
    selectedSet.insert(selectedSet.end(), newSelection.begin(), newSelection.end());
    sort_selection();
    calculate_aabb();
}

void DrawingProgramSelection::set_to_selection(const std::vector<CanvasComponentContainer::ObjInfo*>& newSelection) {
    selectedSet = newSelection;
    sort_selection();
    calculate_aabb();
}

void DrawingProgramSelection::sort_selection() {
    std::vector<DrawingProgramLayerListItem*> flattenedLayerList = drawP.layerMan.get_flattened_layer_list();
    std::unordered_map<DrawingProgramLayerListItem*, size_t> flattenedLayerListOrder;
    for(size_t i = 0; i < flattenedLayerList.size(); i++)
        flattenedLayerListOrder[flattenedLayerList[i]] = flattenedLayerList.size() - 1 - i;
    std::sort(selectedSet.begin(), selectedSet.end(), [&](auto& a, auto& b) {
        return (flattenedLayerListOrder[a->obj->parentLayer] < flattenedLayerListOrder[b->obj->parentLayer]) || (a->obj->parentLayer == b->obj->parentLayer && a->pos < b->pos);
    });
}

void DrawingProgramSelection::calculate_aabb() {
    if(is_something_selected()) {
        initialSelectionAABB = (*selectedSet.begin())->obj->get_world_bounds();
        for(auto& c : selectedSet)
            initialSelectionAABB.include_aabb_in_bounds(c->obj->get_world_bounds());
        rotateData.centerPos = initialSelectionAABB.center();
    }
}

bool DrawingProgramSelection::is_something_selected() {
    return !selectedSet.empty();
}

bool DrawingProgramSelection::is_selected(CanvasComponentContainer::ObjInfo* objToCheck) {
    return std::find(selectedSet.begin(), selectedSet.end(), objToCheck) != selectedSet.end();
}

void DrawingProgramSelection::reset_all() {
    selectedSet.clear();
    reset_transform_data();
}

void DrawingProgramSelection::reset_transform_data() {
    selectionTransformCoords = CoordSpaceHelperTransform();
    transformOpHappening = TransformOperation::NONE;
    translateData = TranslationData();
    scaleData = ScaleData();
    rotateData = RotationData();
}

bool DrawingProgramSelection::mouse_collided_with_selection_aabb() {
    return SCollision::collide(camSpaceSelection, drawP.world.main.input.mouse.pos);
}

bool DrawingProgramSelection::mouse_collided_with_scale_point() {
    return SCollision::collide(SCollision::Circle(scaleData.handlePoint, drawP.drag_point_radius()), drawP.world.main.input.mouse.pos);
}

bool DrawingProgramSelection::mouse_collided_with_rotate_center_handle_point() {
    return SCollision::collide(SCollision::Circle(rotateData.centerHandlePoint, drawP.drag_point_radius()), drawP.world.main.input.mouse.pos);
}

bool DrawingProgramSelection::mouse_collided_with_rotate_handle_point() {
    return SCollision::collide(SCollision::Circle(rotateData.handlePoint, drawP.drag_point_radius() * ROTATION_POINT_RADIUS_MULTIPLIER), drawP.world.main.input.mouse.pos);
}

void DrawingProgramSelection::commit_transform_selection() {
    if(!is_empty_transform()) {
        std::vector<WorldUndoManager::UndoObjectID> undoIDList;
        std::vector<std::pair<CoordSpaceHelper, CoordSpaceHelper>> transformDataList;
        for(auto& comp : selectedSet) {
            undoIDList.emplace_back(drawP.world.undo.get_undoid_from_netid(comp->obj.get_net_id()));
            auto& transformData = transformDataList.emplace_back();
            transformData.first = comp->obj->coords;
            comp->obj->coords = selectionTransformCoords.other_coord_space_from_this_space(comp->obj->coords);
            transformData.second = comp->obj->coords;
            comp->obj->commit_transform_dont_invalidate_cache();
        }

        class TransformCanvasComponentsWorldUndoAction : public WorldUndoAction {
            public:
                TransformCanvasComponentsWorldUndoAction(std::vector<WorldUndoManager::UndoObjectID> initUndoIDs, std::vector<std::pair<CoordSpaceHelper, CoordSpaceHelper>> initTransformData):
                    undoIDs(std::move(initUndoIDs)),
                    transformData(std::move(initTransformData))
                {}
                std::string get_name() const override {
                    return "Transform Canvas Components";
                }
                bool undo(WorldUndoManager& undoMan) override {
                    std::vector<NetworkingObjects::NetObjID> toEditIDs;
                    if(!undoMan.fill_netid_list_from_undoid_list(toEditIDs, undoIDs))
                        return false;

                    std::vector<CanvasComponentContainer::ObjInfo*> transformSet;

                    for(size_t i = 0; i < toEditIDs.size(); i++) {
                        auto objPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<CanvasComponentContainer>(toEditIDs[i]);
                        objPtr->coords = transformData[i].first;
                        objPtr->commit_transform(undoMan.world.drawProg);
                        transformSet.emplace_back(&(*objPtr->objInfo));
                    }

                    undoMan.world.drawProg.send_transforms_for(transformSet);
                    return true;
                }
                bool redo(WorldUndoManager& undoMan) override {
                    std::vector<NetworkingObjects::NetObjID> toEditIDs;
                    if(!undoMan.fill_netid_list_from_undoid_list(toEditIDs, undoIDs))
                        return false;

                    std::vector<CanvasComponentContainer::ObjInfo*> transformSet;

                    for(size_t i = 0; i < toEditIDs.size(); i++) {
                        auto objPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<CanvasComponentContainer>(toEditIDs[i]);
                        objPtr->coords = transformData[i].second;
                        objPtr->commit_transform(undoMan.world.drawProg);
                        transformSet.emplace_back(&(*objPtr->objInfo));
                    }

                    undoMan.world.drawProg.send_transforms_for(transformSet);
                    return true;
                }
                void scale_up(const WorldScalar& scaleUpAmount) override {
                    for(auto& [oldTransform, newTransform] : transformData) {
                        oldTransform.scale_about(WorldVec{0, 0}, scaleUpAmount, true);
                        newTransform.scale_about(WorldVec{0, 0}, scaleUpAmount, true);
                    }
                }
                ~TransformCanvasComponentsWorldUndoAction() {}

                std::vector<WorldUndoManager::UndoObjectID> undoIDs;
                std::vector<std::pair<CoordSpaceHelper, CoordSpaceHelper>> transformData;
        };

        drawP.world.undo.push(std::make_unique<TransformCanvasComponentsWorldUndoAction>(std::move(undoIDList), std::move(transformDataList)));
    }

    drawP.send_transforms_for(selectedSet);

    reset_transform_data();
    calculate_aabb();
}

void DrawingProgramSelection::update() {
    if(is_something_selected()) {
        selectionRectPoints[0] = selectionTransformCoords.from_space_world(initialSelectionAABB.min);
        selectionRectPoints[1] = selectionTransformCoords.from_space_world(initialSelectionAABB.bottom_left());
        selectionRectPoints[2] = selectionTransformCoords.from_space_world(initialSelectionAABB.max);
        selectionRectPoints[3] = selectionTransformCoords.from_space_world(initialSelectionAABB.top_right());

        scaleData.handlePoint = drawP.world.drawData.cam.c.to_space(initialSelectionAABB.max);
        rotateData.centerHandlePoint = drawP.world.drawData.cam.c.to_space(rotateData.centerPos);
        rotateData.handlePoint = get_rotation_point_pos_from_angle(rotateData.rotationAngle);

        rebuild_cam_space();

        if(drawP.is_actual_selection_tool(drawP.drawTool->get_type())) {
            switch(transformOpHappening) {
                case TransformOperation::NONE:
                    if(drawP.controls.leftClick && !drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held && !drawP.world.main.input.key(InputManager::KEY_GENERIC_LALT).held) {
                        if(mouse_collided_with_scale_point()) {
                            scaleData.currentPos = scaleData.startPos = selectionTransformCoords.from_space_world(initialSelectionAABB.max);
                            scaleData.centerPos = selectionTransformCoords.from_space_world(initialSelectionAABB.center());
                            transformOpHappening = TransformOperation::SCALE;
                        }
                        else if(mouse_collided_with_rotate_center_handle_point())
                            transformOpHappening = TransformOperation::ROTATE_RELOCATE_CENTER;
                        else if(mouse_collided_with_rotate_handle_point()) {
                            rotateData.rotationAngle = 0.0;
                            transformOpHappening = TransformOperation::ROTATE;
                        }
                        else if(mouse_collided_with_selection_aabb()) {
                            translateData.startPos = drawP.world.get_mouse_world_pos();
                            transformOpHappening = TransformOperation::TRANSLATE;
                        }
                    }
                    break;
                case TransformOperation::TRANSLATE:
                    if(drawP.controls.leftClickHeld)
                        selectionTransformCoords = CoordSpaceHelperTransform(drawP.world.get_mouse_world_pos() - translateData.startPos);
                    else {
                        commit_transform_selection();
                        return;
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
                            WorldMultiplier scaleAmount = WorldMultiplier(centerToScaleStartNorm) / WorldMultiplier(centerToScaleCurrentNorm);

                            selectionTransformCoords = CoordSpaceHelperTransform(scaleData.centerPos, scaleAmount);
                        }
                    }
                    else {
                        commit_transform_selection();
                        return;
                    }
                    break;
                case TransformOperation::ROTATE_RELOCATE_CENTER:
                    if(drawP.controls.leftClickHeld)
                        rotateData.centerPos = drawP.world.get_mouse_world_pos();
                    else
                        transformOpHappening = TransformOperation::NONE;
                    break;
                case TransformOperation::ROTATE:
                    if(drawP.controls.leftClickHeld) {
                        Vector2f rotationPointDiff = drawP.world.main.input.mouse.pos - rotateData.centerHandlePoint;
                        rotateData.rotationAngle = std::atan2(rotationPointDiff.y(), rotationPointDiff.x());
                        selectionTransformCoords = CoordSpaceHelperTransform(rotateData.centerPos, rotateData.rotationAngle);
                    }
                    else {
                        commit_transform_selection();
                        return;
                    }
                    break;
            }
        }
        else if(transformOpHappening != TransformOperation::NONE) {
            commit_transform_selection();
            return;
        }

        if(drawP.world.main.input.key(InputManager::KEY_GENERIC_ESCAPE).pressed)
            deselect_all();
        else if(drawP.world.main.input.key(InputManager::KEY_DRAW_DELETE).pressed)
            delete_all();
        else if(drawP.world.main.input.key(InputManager::KEY_COPY).pressed && !is_being_transformed())
            selection_to_clipboard();
        else if(drawP.world.main.input.key(InputManager::KEY_CUT).pressed && !is_being_transformed()) {
            selection_to_clipboard();
            delete_all();
        }
        else if(drawP.world.main.input.key(InputManager::KEY_PASTE).pressed) {
            deselect_all();
            paste_clipboard(drawP.world.main.input.mouse.pos);
        }
    }
    else if(drawP.world.main.input.key(InputManager::KEY_PASTE).pressed && !drawP.prevent_undo_or_redo())
        paste_clipboard(drawP.world.main.input.mouse.pos);
    else if(drawP.world.main.input.key(InputManager::KEY_GENERIC_ESCAPE).pressed)
        drawP.switch_to_tool(DrawingProgramToolType::EDIT, true);
}

void DrawingProgramSelection::deselect_all() {
    if(is_something_selected()) {
        commit_transform_selection();
        for(auto& obj : selectedSet)
            drawP.drawCache.add_component(obj);
        reset_all();
    }
}

void DrawingProgramSelection::delete_all() {
    if(is_something_selected()) {
        auto selectedSetTemp = selectedSet;
        reset_all(); // Clear set before erasing, since calling erase_component_set will run a check to see if each object is selected and erase them one by one, which is slower
        drawP.layerMan.erase_component_container(selectedSetTemp);
    }
}

void DrawingProgramSelection::erase_component(CanvasComponentContainer::ObjInfo* objToCheck) {
    std::erase(selectedSet, objToCheck);
    if(selectedSet.empty())
        reset_all();
}

CanvasComponentContainer::ObjInfo* DrawingProgramSelection::get_front_object_colliding_with_in_editing_layer(const SCollision::ColliderCollection<float>& cC) {
    auto cCWorld = drawP.world.drawData.cam.c.collider_to_world<SCollision::ColliderCollection<WorldScalar>, SCollision::ColliderCollection<float>>(cC);
    CanvasComponentContainer::ObjInfo* p = nullptr;
    for(auto& c : selectedSet) {
        if(drawP.layerMan.component_passes_layer_selector(c, DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED) && (!p || c->pos >= p->pos) && c->obj->collides_with(drawP.world.drawData.cam.c, cCWorld, cC))
            p = c;
    }
    return p;
}

void DrawingProgramSelection::selection_to_clipboard() {
    auto& clipboard = drawP.world.main.clipboard;
    std::unordered_set<NetworkingObjects::NetObjID> resourceSet;
    for(auto& c : selectedSet)
        c->obj->get_comp().get_used_resources(resourceSet);
    clipboard.components.clear();
    for(auto& c : selectedSet)
        clipboard.components.emplace_back(c->obj->get_data_copy());
    clipboard.pos = initialSelectionAABB.center();
    clipboard.inverseScale = drawP.world.drawData.cam.c.inverseScale;
    clipboard.resources = drawP.world.rMan.copy_resource_set_to_map(resourceSet);
}

void DrawingProgramSelection::paste_clipboard(Vector2f pasteScreenPos) {
    if(drawP.layerMan.is_a_layer_being_edited()) {
        auto& clipboard = drawP.world.main.clipboard;

        if(!drawP.is_selection_allowing_tool(drawP.drawTool->get_type()))
            drawP.switch_to_tool(DrawingProgramToolType::EDIT);

        std::vector<std::pair<CanvasComponentContainer::ObjInfoIterator, CanvasComponentContainer*>> placedComponents;
        WorldVec mousePos = drawP.world.drawData.cam.c.from_space(pasteScreenPos);
        WorldVec moveVec = drawP.world.main.clipboard.pos - mousePos;
        WorldMultiplier scaleMultiplier = WorldMultiplier(drawP.world.main.clipboard.inverseScale) / WorldMultiplier(drawP.world.drawData.cam.c.inverseScale);
        WorldScalar scaleLimit(0.001);
        for(auto& c : drawP.world.main.clipboard.components) {
            if((c->coords.inverseScale / scaleMultiplier) < scaleLimit) {
                Logger::get().log("WORLDFATAL", "Some pasted objects will be too small! Scale up the canvas first (zoom in alot)");
                return;
            }
        }

        std::unordered_map<NetworkingObjects::NetObjID, NetworkingObjects::NetObjID> resourceRemapIDs;
        for(auto& r : clipboard.resources)
            resourceRemapIDs[r.first] = drawP.world.rMan.add_resource(r.second).get_net_id();
        for(auto& c : clipboard.components) {
            CanvasComponentContainer* newComponentContainer = new CanvasComponentContainer(drawP.world.netObjMan, *c);
            newComponentContainer->get_comp().remap_resource_ids(resourceRemapIDs);
            newComponentContainer->coords.translate(-moveVec);
            newComponentContainer->coords.scale_about(mousePos, scaleMultiplier);
            placedComponents.emplace_back(drawP.layerMan.get_edited_layer_end_iterator(), newComponentContainer);
        }
        auto newlyInsertedObjectIts = drawP.layerMan.add_many_components_to_layer_being_edited(placedComponents);
        std::vector<CanvasComponentContainer::ObjInfo*> compSetInserted;
        for(auto& it : newlyInsertedObjectIts) {
            drawP.drawCache.erase_component(&(*it));
            compSetInserted.emplace_back(&(*it));
        }
        set_to_selection(compSetInserted);
        //drawP.add_undo_place_components(compSetInserted);
    }
}

bool DrawingProgramSelection::is_being_transformed() {
    return transformOpHappening != TransformOperation::NONE;
}

bool DrawingProgramSelection::is_empty_transform() {
    return transformOpHappening == TransformOperation::NONE || selectionTransformCoords.is_identity();
}

std::unordered_set<CanvasComponentContainer::ObjInfo*> DrawingProgramSelection::get_selection_as_set() {
    return std::unordered_set<CanvasComponentContainer::ObjInfo*>(selectedSet.begin(), selectedSet.end());
}

void DrawingProgramSelection::rebuild_cam_space() {
    SCollision::ColliderCollection<WorldScalar> collideRectWorld;
    collideRectWorld.triangle.emplace_back(selectionRectPoints[0], selectionRectPoints[1], selectionRectPoints[2]);
    collideRectWorld.triangle.emplace_back(selectionRectPoints[0], selectionRectPoints[2], selectionRectPoints[3]);
    collideRectWorld.recalculate_bounds();

    camSpaceSelection = drawP.world.drawData.cam.c.world_collider_to_coords<SCollision::ColliderCollection<float>>(collideRectWorld);
}

Vector2f DrawingProgramSelection::get_rotation_point_pos_from_angle(double angle) {
    return Vector2f{rotateData.centerHandlePoint + ROTATION_POINTS_DISTANCE * drawP.world.main.toolbar.final_gui_scale() * Vector2f{std::cos(angle), std::sin(angle)}};
}

void DrawingProgramSelection::draw_components(SkCanvas* canvas, const DrawData& drawData) {
    if(is_something_selected()) {
        selectionRectPoints[0] = selectionTransformCoords.from_space_world(initialSelectionAABB.min);
        selectionRectPoints[1] = selectionTransformCoords.from_space_world(initialSelectionAABB.bottom_left());
        selectionRectPoints[2] = selectionTransformCoords.from_space_world(initialSelectionAABB.max);
        selectionRectPoints[3] = selectionTransformCoords.from_space_world(initialSelectionAABB.top_right());

        scaleData.handlePoint = drawP.world.drawData.cam.c.to_space(selectionTransformCoords.from_space_world(initialSelectionAABB.max));
        rotateData.centerHandlePoint = drawP.world.drawData.cam.c.to_space(selectionTransformCoords.from_space_world(rotateData.centerPos));
        rotateData.handlePoint = get_rotation_point_pos_from_angle(rotateData.rotationAngle);

        rebuild_cam_space();

        DrawData selectionDrawData = drawData;
        selectionDrawData.cam.c = selectionTransformCoords.other_coord_space_to_this_space(selectionDrawData.cam.c);
        selectionDrawData.cam.set_viewing_area(drawP.world.main.window.size.cast<float>());
        selectionDrawData.refresh_draw_optimizing_values();

        SkImageInfo imgInfo = canvas->imageInfo();
        sk_sp<SkSurface> surface;
        #ifdef USE_SKIA_BACKEND_GRAPHITE
            surface = SkSurfaces::RenderTarget(drawP.world.main.window.recorder(), imgInfo, skgpu::Mipmapped::kNo, drawP.world.main.window.defaultMSAASurfaceProps);
        #elif USE_SKIA_BACKEND_GANESH
            surface = SkSurfaces::RenderTarget(drawP.world.main.window.ctx.get(), skgpu::Budgeted::kNo, imgInfo, drawP.world.main.window.defaultMSAASampleCount, &drawP.world.main.window.defaultMSAASurfaceProps);
        #endif
        if(!surface)
            throw std::runtime_error("[DrawingProgramSelection::draw_components] Could not make temporary surface");

        for(auto& c : selectedSet)
            c->obj->draw(canvas, selectionDrawData);

        SkPaint glowBlurP;
        glowBlurP.setImageFilter(SkImageFilters::Blur(5, 5, nullptr));

        canvas->drawImage(surface->makeTemporaryImage(), 0, 0, SkSamplingOptions{SkFilterMode::kLinear}, &glowBlurP);
        canvas->drawImage(surface->makeTemporaryImage(), 0, 0, SkSamplingOptions{SkFilterMode::kLinear}, nullptr);
    }
}

void DrawingProgramSelection::draw_gui(SkCanvas* canvas, const DrawData& drawData) {
    if(is_something_selected() && !camSpaceSelection.triangle.empty()) {
        SkPathBuilder selectionRectPath;
        selectionRectPath.moveTo(convert_vec2<SkPoint>(camSpaceSelection.triangle[0].p[0]));
        selectionRectPath.lineTo(convert_vec2<SkPoint>(camSpaceSelection.triangle[0].p[1]));
        selectionRectPath.lineTo(convert_vec2<SkPoint>(camSpaceSelection.triangle[0].p[2]));
        selectionRectPath.lineTo(convert_vec2<SkPoint>(camSpaceSelection.triangle[1].p[2]));
        selectionRectPath.close();
        SkPaint p{SkColor4f{0.3f, 0.6f, 0.9f, 0.4f}};
        canvas->drawPath(selectionRectPath.detach(), p);

        if(drawP.is_actual_selection_tool(drawP.drawTool->get_type())) {
            if(transformOpHappening == TransformOperation::NONE || transformOpHappening == TransformOperation::ROTATE || transformOpHappening == TransformOperation::ROTATE_RELOCATE_CENTER)
                drawP.draw_drag_circle(canvas, rotateData.centerHandlePoint, {0.9f, 0.5f, 0.1f, 1.0f}, drawData);
            if(transformOpHappening == TransformOperation::NONE || transformOpHappening == TransformOperation::ROTATE)
                drawP.draw_drag_circle(canvas, rotateData.handlePoint, {0.9f, 0.5f, 0.1f, 1.0f}, drawData, ROTATION_POINT_RADIUS_MULTIPLIER);
            if(transformOpHappening == TransformOperation::NONE || transformOpHappening == TransformOperation::SCALE)
                drawP.draw_drag_circle(canvas, scaleData.handlePoint, {0.1f, 0.9f, 0.9f, 1.0f}, drawData);
        }
    }
}
