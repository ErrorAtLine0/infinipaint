#include "DrawingProgramSelection.hpp"
#include "DrawingProgram.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"

DrawingProgramSelection::DrawingProgramSelection(DrawingProgram& initDrawP):
    cache(initDrawP),
    drawP(initDrawP)
{}

void DrawingProgramSelection::add_from_cam_coord_collider_to_selection(const SCollision::ColliderCollection<float>& cC) {
    auto cCWorld = drawP.world.drawData.cam.c.collider_to_world<SCollision::ColliderCollection<WorldScalar>, SCollision::ColliderCollection<float>>(cC);

    drawP.compCache.disableRefresh = true;

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
        selectionAABB = (*selectedSet.begin())->obj->worldAABB.value();
        for(auto& c : selectedSet)
            selectionAABB.include_aabb_in_bounds(c->obj->worldAABB.value());
    }
}

void DrawingProgramSelection::deselect_all() {
    for(auto& obj : selectedSet) {
        obj->obj->coords = selectionTransformCoords.other_coord_space_from_this_space(obj->obj->coords);
        obj->obj->final_update(drawP);
    }
    selectedSet.clear();
    cache.clear();
    selectionTransformCoords = CoordSpaceHelper();
    drawP.compCache.force_rebuild(drawP.components.client_list());
}

void DrawingProgramSelection::update() {
    if(is_something_selected()) {
        if(cache.get_unsorted_component_list().size() >= 1000 && (std::chrono::steady_clock::now() - cache.get_last_bvh_build_time()) >= std::chrono::seconds(5))
            cache.force_rebuild(std::vector<CollabListType::ObjectInfoPtr>(selectedSet.begin(), selectedSet.end()));
        if(drawP.world.main.input.key(InputManager::KEY_DRAW_UNSELECT).pressed)
            deselect_all();
        if(drawP.world.main.input.key(InputManager::KEY_TEXT_UP).held)
            selectionTransformCoords.translate(WorldVec{0, -drawP.world.drawData.cam.c.inverseScale});
        if(drawP.world.main.input.key(InputManager::KEY_TEXT_DOWN).held)
            selectionTransformCoords.translate(WorldVec{0, drawP.world.drawData.cam.c.inverseScale});
        if(drawP.world.main.input.key(InputManager::KEY_TEXT_LEFT).held)
            selectionTransformCoords.translate(WorldVec{-drawP.world.drawData.cam.c.inverseScale, 0});
        if(drawP.world.main.input.key(InputManager::KEY_TEXT_RIGHT).held)
            selectionTransformCoords.translate(WorldVec{drawP.world.drawData.cam.c.inverseScale, 0});
        if(drawP.world.main.input.key(InputManager::KEY_TEXT_CTRL).held)
            selectionTransformCoords.scale_about_double(selectionTransformCoords.pos, 1.01);
        if(drawP.world.main.input.key(InputManager::KEY_TEXT_SHIFT).held)
            selectionTransformCoords.scale_about_double(selectionTransformCoords.pos, 0.99);
    }
}

bool DrawingProgramSelection::mouse_collided_with_selection_aabb() {
    return false;
}

void DrawingProgramSelection::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(is_something_selected()) {
        DrawData selectionDrawData = drawData;
        selectionDrawData.cam.c = selectionTransformCoords.other_coord_space_to_this_space(selectionDrawData.cam.c);
        selectionDrawData.cam.set_viewing_area(drawP.world.main.window.size.cast<float>());
        selectionDrawData.refresh_draw_optimizing_values();

        cache.refresh_all_draw_cache(selectionDrawData);
        cache.draw_components_to_canvas(canvas, selectionDrawData);

        canvas->save();
        selectionTransformCoords.transform_sk_canvas(canvas, selectionDrawData);
        Vector2f minRectPos = selectionTransformCoords.to_space(selectionAABB.min);
        Vector2f maxRectPos = selectionTransformCoords.to_space(selectionAABB.max);
        SkPaint p{SkColor4f{0.0f, 0.0f, 1.0f, 0.3f}};
        canvas->drawRect(SkRect::MakeLTRB(minRectPos.x(), minRectPos.y(), maxRectPos.x(), maxRectPos.y()), p);
        canvas->restore();
    }
}
