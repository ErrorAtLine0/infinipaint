#include "EraserTool.hpp"
#include "DrawingProgram.hpp"
#include "../Server/CommandList.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "../SharedTypes.hpp"
#include <ranges>
#include "../DrawCollision.hpp"

EraserTool::EraserTool(DrawingProgram& initDrawP):
    drawP(initDrawP)
{
}

void EraserTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("eraser tool");
    t.gui.text_label_centered("Eraser");
    t.gui.slider_scalar_field("relwidth", "Size", &drawP.controls.relativeWidth, 3.0f, 40.0f);
    t.gui.pop_id();
}

void EraserTool::reset_tool() {
    if(!erasedComponents.empty()) {
        DrawingProgramCache::move_components_from_bvh_nodes_to_set(erasedComponents, erasedBVHNodes);

        std::unordered_set<ServerClientID> idsToErase;
        for(auto& c : erasedComponents)
            idsToErase.emplace(c->id);
        DrawComponent::client_send_erase_set(drawP, idsToErase);
        drawP.components.client_erase_set(erasedComponents);

        drawP.world.undo.push(UndoManager::UndoRedoPair{
            [&, erasedComponents = erasedComponents]() {
                std::vector<CollabListType::ObjectInfoPtr> sortedObjects(erasedComponents.begin(), erasedComponents.end());
                std::sort(sortedObjects.begin(), sortedObjects.end(), [](auto& a, auto& b) {
                    return a->pos < b->pos;
                });

                for(auto& comp : sortedObjects)
                    if(comp->obj->collabListInfo.lock())
                        return false;

                drawP.components.client_insert_ordered_vector(sortedObjects);
                DrawComponent::client_send_place_many(drawP, sortedObjects);

                drawP.reset_tools();

                return true;
            },
            [&, erasedComponents = erasedComponents]() {
                for(auto& comp : erasedComponents)
                    if(!comp->obj->collabListInfo.lock())
                        return false;

                std::unordered_set<ServerClientID> idsToErase;
                for(auto& c : erasedComponents)
                    idsToErase.emplace(c->id);

                DrawComponent::client_send_erase_set(drawP, idsToErase);
                drawP.components.client_erase_set(erasedComponents);

                drawP.reset_tools();

                drawP.force_rebuild_cache();

                return true;
            }
        });
    }
    erasedComponents.clear();
    erasedBVHNodes.clear();
}

void EraserTool::tool_update() {
    if(drawP.controls.cursorHoveringOverCanvas)
        drawP.world.main.input.hideCursor = true;

    if(drawP.controls.leftClickHeld) {
        const Vector2f& prevMousePos = drawP.controls.leftClick ? drawP.world.main.input.mouse.pos : drawP.world.main.input.mouse.lastPos;
        SCollision::ColliderCollection<float> cC;
        SCollision::generate_wide_line(cC, prevMousePos, drawP.world.main.input.mouse.pos, drawP.controls.relativeWidth * 2.0f, true);
        auto cCWorld = drawP.world.drawData.cam.c.collider_to_world<SCollision::ColliderCollection<WorldScalar>, SCollision::ColliderCollection<float>>(cC);

        drawP.compCache.traverse_bvh_erase_function(cCWorld.bounds, [&](const auto& bvhNode, auto& comps) {
            if(bvhNode &&
               SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.min)) &&
               SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.max)) &&
               SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.top_right())) &&
               SCollision::collide(cC, drawP.world.drawData.cam.c.to_space(bvhNode->bounds.bottom_left()))) {
                erasedBVHNodes.emplace(bvhNode);
                drawP.compCache.invalidate_cache_at_aabb_before_pos(bvhNode->bounds, 0);
                return true;
            }
            std::erase_if(comps, [&](auto& c) {
                if(c->obj->collides_with(drawP.world.drawData.cam.c, cCWorld, cC, drawP.colliderAllocated)) {
                    erasedComponents.emplace(c);
                    drawP.compCache.invalidate_cache_at_aabb_before_pos(c->obj->worldAABB.value(), c->obj->collabListInfo.lock()->pos);
                    return true;
                }
                return false;
            });
            return false;
        });
    }
    else
        reset_tool();
}

void EraserTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(!drawData.main->toolbar.io->hoverObstructed) {
        SkColor4f c = drawP.world.main.canvasTheme.toolFrontColor;
        c.fA = 0.5f;

        SkPaint linePaint;
        linePaint.setColor4f(c);
        linePaint.setStyle(SkPaint::kStroke_Style);
        linePaint.setStrokeCap(SkPaint::kRound_Cap);
        linePaint.setStrokeWidth(drawP.controls.relativeWidth * 2.0f);
        SkPath erasePath;
        erasePath.moveTo(convert_vec2<SkPoint>(drawData.main->input.mouse.lastPos));
        erasePath.lineTo(convert_vec2<SkPoint>(drawData.main->input.mouse.pos));
        canvas->drawPath(erasePath, linePaint);
    }
}
