#include "EditTool.hpp"
#include <chrono>
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "../SharedTypes.hpp"
#include <cereal/types/vector.hpp>
#include <memory>

#include "TextBoxEditTool.hpp"
#include "RectDrawEditTool.hpp"
#include "ImageEditTool.hpp"
#include "EllipseDrawEditTool.hpp"

EditTool::EditTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType EditTool::get_type() {
    return DrawingProgramToolType::EDIT;
}

void EditTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    if(controls.isEditing) {
        std::shared_ptr<DrawComponent> a = controls.compToEdit.lock();
        if(a) {
            bool editHappened = controls.compEditTool->edit_gui(a);
            if(editHappened) {
                a->commit_update(drawP);
                a->client_send_update(drawP);
            }
        }
    }
    else {
        t.gui.push_id("edit tool");
        t.gui.text_label_centered("Edit");
        t.gui.pop_id();
    }
}

void EditTool::add_point_handle(const HandleData& handle) {
    controls.pointHandles.emplace_back(handle);
}

void EditTool::reset_tool() {
    auto a = controls.compToEdit.lock();
    if(a && a->collabListInfo.lock()) {
        controls.compEditTool->commit_edit_updates(a, controls.prevData);
        a->commit_update(drawP);
        a->client_send_update(drawP);
    }
    controls.compToEdit.reset();
    controls.pointHandles.clear();
    controls.pointDragging = nullptr;
    controls.isEditing = false;
}

void EditTool::edit_start(const std::shared_ptr<DrawComponent>& comp) {
    controls.isEditing = true;
    switch(comp->get_type()) {
        case DRAWCOMPONENT_TEXTBOX: {
            controls.compEditTool = std::make_unique<TextBoxEditTool>(drawP);
            break;
        }
        case DRAWCOMPONENT_ELLIPSE: {
            controls.compEditTool = std::make_unique<EllipseDrawEditTool>(drawP);
            break;
        }
        case DRAWCOMPONENT_RECTANGLE: {
            controls.compEditTool = std::make_unique<RectDrawEditTool>(drawP);
            break;
        }
        case DRAWCOMPONENT_IMAGE: {
            controls.compEditTool = std::make_unique<ImageEditTool>(drawP);
            break;
        }
        default: {
            controls.isEditing = false;
            break;
        }
    }
    if(controls.isEditing) {
        controls.compToEdit = comp;
        controls.compEditTool->edit_start(*this, comp, controls.prevData);
        comp->lastUpdateTime = std::chrono::steady_clock::now();
    }
}

void EditTool::tool_update() {
    if(!controls.isEditing) {
        SCollision::AABB<WorldScalar> mouseAABB{drawP.world.get_mouse_world_pos() - WorldVec{0.5f, 0.5f}, drawP.world.get_mouse_world_pos() + WorldVec{0.5f, 0.5f}};
        SCollision::ColliderCollection<WorldScalar> cMouseAABB;
        cMouseAABB.aabb.emplace_back(mouseAABB);
        cMouseAABB.recalculate_bounds();

        if(drawP.controls.leftClick) {
            CollabListType::ObjectInfoPtr lastSelectedObj;
            uint64_t lastPos = 0;

            drawP.compCache.traverse_bvh_run_function(mouseAABB, [&](const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode, const std::vector<CollabListType::ObjectInfoPtr>& components) {
                for(auto& c : components) {
                    if(c->pos >= lastPos && c->obj->collides_with_world_coords(drawP.world.drawData.cam.c, cMouseAABB)) {
                        lastSelectedObj = c;
                        lastPos = c->pos;
                    }
                }
                return true;
            });

            if(lastSelectedObj)
                edit_start(lastSelectedObj->obj);
        }
    }
    else {
        SCollision::Circle<float> mouseCircle{drawP.world.main.input.mouse.pos, 1.0f};
        SCollision::ColliderCollection<float> cMouseCircle;
        cMouseCircle.circle.emplace_back(mouseCircle);
        cMouseCircle.recalculate_bounds();

        std::shared_ptr<DrawComponent> a = controls.compToEdit.lock();
        if(a) {
            bool isMovingPoint = false;
            bool clickedAway = false;
            if(!controls.pointDragging && drawP.controls.leftClick) {
                for(HandleData& h : controls.pointHandles) {
                    if(SCollision::collide(mouseCircle, SCollision::Circle<float>(drawP.world.drawData.cam.c.to_space(a->coords.from_space(*h.p)), DRAG_POINT_RADIUS))) {
                        controls.pointDragging = &h;
                        isMovingPoint = true;
                        break;
                    }
                }
                if(!isMovingPoint && !a->collides_with_cam_coords(drawP.world.drawData.cam.c, cMouseCircle))
                    clickedAway = true;
            }
            else if(controls.pointDragging) {
                if(drawP.controls.leftClickHeld) {
                    Vector2f newPos = a->coords.get_mouse_pos(drawP.world);
                    if(newPos != *controls.pointDragging->p) {
                        if(controls.pointDragging->min)
                            newPos = cwise_vec_max(*controls.pointDragging->min, newPos);
                        if(controls.pointDragging->max)
                            newPos = cwise_vec_min(*controls.pointDragging->max, newPos);
                        *controls.pointDragging->p = newPos;
                        a->commit_update(drawP);
                        a->client_send_update(drawP);
                    }
                    isMovingPoint = true;
                }
                else
                    controls.pointDragging = nullptr;
            }

            a->lastUpdateTime = std::chrono::steady_clock::now();

            bool shouldNotReset = controls.compEditTool->edit_update(a);
            if(!shouldNotReset || clickedAway)
                reset_tool();
        }
        else
            reset_tool();
    }
}

bool EditTool::prevent_undo_or_redo() {
    return controls.isEditing;
}

void EditTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(controls.isEditing) {
        std::shared_ptr<DrawComponent> a = controls.compToEdit.lock();
        if(a) {
            for(HandleData& h : controls.pointHandles)
                drawP.draw_drag_circle(canvas, drawData.cam.c.to_space((a->coords.from_space(*h.p))), {0.1f, 0.9f, 0.9f, 1.0f}, drawData);
        }
    }
}
