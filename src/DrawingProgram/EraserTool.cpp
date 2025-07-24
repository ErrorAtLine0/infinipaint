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
        for(auto& bvhNode : erasedBVHNodes)
            move_erased_components_from_bvh_nodes_recursive(bvhNode);

        for(auto& c : erasedComponents)
            c->obj->client_send_erase(drawP);
        drawP.components.client_erase_set(erasedComponents);

        drawP.world.undo.push(UndoManager::UndoRedoPair{
            [&, erasedCompVal = erasedComponents]() {
                std::vector<CollabListType::ObjectInfoPtr> sortedObjects(erasedCompVal.begin(), erasedCompVal.end());
                std::sort(sortedObjects.begin(), sortedObjects.end(), [](auto& a, auto& b) {
                    return a->pos < b->pos;
                });

                for(auto& comp : sortedObjects)
                    if(comp->obj->collabListInfo.lock())
                        return false;

                drawP.components.client_insert_ordered_vector(sortedObjects);

                for(auto& comp : sortedObjects)
                    comp->obj->client_send_place(drawP);

                drawP.reset_tools();

                return true;
            },
            [&, erasedCompVal = erasedComponents]() {
                for(auto& comp : erasedCompVal)
                    if(!comp->obj->collabListInfo.lock())
                        return false;

                for(auto& comp : erasedCompVal)
                    comp->obj->client_send_erase(drawP);

                drawP.components.client_erase_set(erasedCompVal);

                drawP.reset_tools();

                drawP.compCache.force_rebuild(drawP.components.client_list());

                return true;
            }
        });
    }
    erasedComponents.clear();
    erasedBVHNodes.clear();
    drawP.compCache.disableRefresh = false;
}

void EraserTool::tool_update() {
    if(drawP.controls.cursorHoveringOverCanvas)
        drawP.world.main.input.hideCursor = true;

    if(drawP.controls.leftClickHeld) {
        const Vector2f& prevMousePos = drawP.controls.leftClick ? drawP.world.main.input.mouse.pos : drawP.world.main.input.mouse.lastPos;
        SCollision::ColliderCollection<float> cC;
        SCollision::generate_wide_line(cC, prevMousePos, drawP.world.main.input.mouse.pos, drawP.controls.relativeWidth * 2.0f, true);
        auto cCWorld = drawP.world.drawData.cam.c.collider_to_world<SCollision::ColliderCollection<WorldScalar>, SCollision::ColliderCollection<float>>(cC);

        drawP.compCache.disableRefresh = true;

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

void EraserTool::move_erased_components_from_bvh_nodes_recursive(const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode) {
    for(auto& c : bvhNode->components)
        erasedComponents.emplace(c);
    for(auto& p : bvhNode->children)
        move_erased_components_from_bvh_nodes_recursive(p);
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
