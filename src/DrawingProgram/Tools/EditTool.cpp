#include "EditTool.hpp"
#include <chrono>
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "../../SharedTypes.hpp"
#include <cereal/types/vector.hpp>
#include <memory>

#include "EditTools/TextBoxEditTool.hpp"
#include "EditTools/RectDrawEditTool.hpp"
#include "EditTools/ImageEditTool.hpp"
#include "EditTools/EllipseDrawEditTool.hpp"

EditTool::EditTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType EditTool::get_type() {
    return DrawingProgramToolType::EDIT;
}

void EditTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    if(objInfoBeingEdited) {
        bool editHappened = compEditTool->edit_gui(objInfoBeingEdited);
        if(editHappened)
            objInfoBeingEdited->obj->commit_update(drawP);
    }
    else {
        t.gui.push_id("edit tool");
        t.gui.text_label_centered("Edit");
        t.gui.pop_id();
    }
}

void EditTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    if(erasedComp == objInfoBeingEdited)
        switch_tool(get_type());
}

bool EditTool::right_click_popup_gui(Vector2f popupPos) {
    if(objInfoBeingEdited)
        return compEditTool->right_click_popup_gui(objInfoBeingEdited, popupPos);
    else
        return drawP.selection_action_menu(popupPos);
}

void EditTool::add_point_handle(const HandleData& handle) {
    pointHandles.emplace_back(handle);
}

void EditTool::switch_tool(DrawingProgramToolType newTool) {
    if(objInfoBeingEdited) {
        compEditTool->commit_edit_updates(objInfoBeingEdited, prevData);
        objInfoBeingEdited->obj->commit_update(drawP);
        objInfoBeingEdited->obj->send_comp_update(drawP, true);
        objInfoBeingEdited = nullptr;
    }
    pointHandles.clear();
    pointDragging = nullptr;

    if(!drawP.is_selection_allowing_tool(newTool))
        drawP.selection.deselect_all();
}

void EditTool::edit_start(CanvasComponentContainer::ObjInfo* comp) {
    bool isEditing = true;
    switch(comp->obj->get_comp().get_type()) {
        case CanvasComponentType::TEXTBOX: {
            compEditTool = std::make_unique<TextBoxEditTool>(drawP);
            break;
        }
        case CanvasComponentType::ELLIPSE: {
            compEditTool = std::make_unique<EllipseDrawEditTool>(drawP);
            break;
        }
        case CanvasComponentType::RECTANGLE: {
            compEditTool = std::make_unique<RectDrawEditTool>(drawP);
            break;
        }
        case CanvasComponentType::IMAGE: {
            compEditTool = std::make_unique<ImageEditTool>(drawP);
            break;
        }
        default: {
            isEditing = false;
            break;
        }
    }
    if(isEditing) {
        objInfoBeingEdited = comp;
        compEditTool->edit_start(*this, objInfoBeingEdited, prevData);
    }
}

bool EditTool::is_editable(CanvasComponentContainer::ObjInfo* comp) {
    return comp->obj->get_comp().get_type() != CanvasComponentType::BRUSHSTROKE;
}

void EditTool::tool_update() {
    if(!objInfoBeingEdited) {
        SCollision::AABB<WorldScalar> mouseAABB{drawP.world.get_mouse_world_pos() - WorldVec{0.5f, 0.5f}, drawP.world.get_mouse_world_pos() + WorldVec{0.5f, 0.5f}};
        SCollision::ColliderCollection<WorldScalar> cMouseAABB;
        cMouseAABB.aabb.emplace_back(mouseAABB);
        cMouseAABB.recalculate_bounds();

        SCollision::AABB<float> camMouseAABB{drawP.world.main.input.mouse.pos - Vector2f{0.5f, 0.5f}, drawP.world.main.input.mouse.pos + Vector2f{0.5f, 0.5f}};
        SCollision::ColliderCollection<float> camCMouseAABB;
        camCMouseAABB.aabb.emplace_back(camMouseAABB);
        camCMouseAABB.recalculate_bounds();

        if(drawP.controls.leftClick) {
            bool modifySelection = !drawP.selection.is_being_transformed();
            if(drawP.world.main.input.mouse.leftClicks >= 2 && !drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held && !drawP.world.main.input.key(InputManager::KEY_GENERIC_LALT).held) {
                CanvasComponentContainer::ObjInfo* selectedObjectToEdit = drawP.selection.get_front_object_colliding_with_in_editing_layer(camCMouseAABB);

                if(selectedObjectToEdit && is_editable(selectedObjectToEdit)) {
                    drawP.selection.deselect_all();
                    edit_start(selectedObjectToEdit);
                    modifySelection = false;
                }
            }
            if(modifySelection) {
                if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held)
                    drawP.selection.add_from_cam_coord_collider_to_selection(camCMouseAABB, DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED, true);
                else if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LALT).held)
                    drawP.selection.remove_from_cam_coord_collider_to_selection(camCMouseAABB, DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED, true);
                else {
                    drawP.selection.deselect_all();
                    drawP.selection.add_from_cam_coord_collider_to_selection(camCMouseAABB, DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED, true);
                }
            }
        }
    }
    else {
        SCollision::Circle<float> mouseCircle{drawP.world.main.input.mouse.pos, 1.0f};
        SCollision::ColliderCollection<float> cMouseCircle;
        cMouseCircle.circle.emplace_back(mouseCircle);
        cMouseCircle.recalculate_bounds();

        bool isMovingPoint = false;
        bool clickedAway = false;
        if(!pointDragging && drawP.controls.leftClick) {
            for(HandleData& h : pointHandles) {
                if(SCollision::collide(mouseCircle, SCollision::Circle<float>(drawP.world.drawData.cam.c.to_space(objInfoBeingEdited->obj->coords.from_space(*h.p)), drawP.drag_point_radius()))) {
                    pointDragging = &h;
                    isMovingPoint = true;
                    break;
                }
            }
            if(!isMovingPoint && !objInfoBeingEdited->obj->collides_with_cam_coords(drawP.world.drawData.cam.c, cMouseCircle))
                clickedAway = true;
        }
        else if(pointDragging) {
            if(drawP.controls.leftClickHeld) {
                Vector2f newPos = objInfoBeingEdited->obj->coords.get_mouse_pos(drawP.world);
                if(newPos != *pointDragging->p) {
                    if(pointDragging->min)
                        newPos = cwise_vec_max((*pointDragging->min + Vector2f{pointDragging->minimumDistanceBetweenBoundsAndPoint, pointDragging->minimumDistanceBetweenBoundsAndPoint}).eval(), newPos);
                    if(pointDragging->max)
                        newPos = cwise_vec_min((*pointDragging->max - Vector2f{pointDragging->minimumDistanceBetweenBoundsAndPoint, pointDragging->minimumDistanceBetweenBoundsAndPoint}).eval(), newPos);
                    *pointDragging->p = newPos;
                    objInfoBeingEdited->obj->commit_update(drawP);
                }
                isMovingPoint = true;
            }
            else
                pointDragging = nullptr;
        }

        objInfoBeingEdited->obj->send_comp_update(drawP, false);

        bool shouldNotReset = compEditTool->edit_update(objInfoBeingEdited);
        if(!shouldNotReset || clickedAway)
            switch_tool(get_type());
    }
}

bool EditTool::prevent_undo_or_redo() {
    return objInfoBeingEdited;
}

void EditTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(objInfoBeingEdited) {
        for(HandleData& h : pointHandles)
            drawP.draw_drag_circle(canvas, drawData.cam.c.to_space((objInfoBeingEdited->obj->coords.from_space(*h.p))), {0.1f, 0.9f, 0.9f, 1.0f}, drawData);
    }
}
