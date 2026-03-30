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
#include "EditTools/BrushEditTool.hpp"

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"

EditTool::EditTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType EditTool::get_type() {
    return DrawingProgramToolType::EDIT;
}

void EditTool::gui_toolbox() {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    Toolbar& t = drawP.world.main.toolbar;
    if(objInfoBeingEdited)
        compEditTool->edit_gui(objInfoBeingEdited);
    else {
        t.gui.push_id("edit tool");
        text_label_centered(t.gui, "Edit");
        t.gui.pop_id();
    }
}

void EditTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    if(erasedComp == objInfoBeingEdited)
        switch_tool(get_type());
}

void EditTool::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    drawP.selection.input_key_callback_modify_selection(key);
}

void EditTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    drawP.selection.input_mouse_button_on_canvas_callback_modify_selection(button);
    if(button.button == InputManager::MouseButton::LEFT) {
        if(button.down) {
            if(!objInfoBeingEdited) {
                WorldVec mouseWorldPos = drawP.world.drawData.cam.c.from_space(button.pos);

                SCollision::AABB<WorldScalar> mouseAABB{mouseWorldPos - WorldVec{0.5f, 0.5f}, mouseWorldPos + WorldVec{0.5f, 0.5f}};
                SCollision::ColliderCollection<WorldScalar> cMouseAABB;
                cMouseAABB.aabb.emplace_back(mouseAABB);
                cMouseAABB.recalculate_bounds();

                SCollision::AABB<float> camMouseAABB{drawP.world.main.input.mouse.pos - Vector2f{0.5f, 0.5f}, button.pos + Vector2f{0.5f, 0.5f}};
                SCollision::ColliderCollection<float> camCMouseAABB;
                camCMouseAABB.aabb.emplace_back(camMouseAABB);
                camCMouseAABB.recalculate_bounds();

                bool modifySelection = !drawP.selection.is_being_transformed();
                if(button.clicks >= 2 && !drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held && !drawP.world.main.input.key(InputManager::KEY_GENERIC_LALT).held) {
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
            else {
                SCollision::Circle<float> mouseCircle{button.pos, 1.0f};
                SCollision::ColliderCollection<float> cMouseCircle;
                cMouseCircle.circle.emplace_back(mouseCircle);
                cMouseCircle.recalculate_bounds();

                bool isMovingPoint = false;
                bool clickedAway = false;

                if(!pointDragging) {
                    for(HandleData& h : pointHandles) {
                        if(SCollision::collide(mouseCircle, SCollision::Circle<float>(drawP.world.drawData.cam.c.to_space(objInfoBeingEdited->obj->coords.from_space(h.coordMatrix * (*h.p))), drawP.drag_point_radius()))) {
                            pointDragging = &h;
                            isMovingPoint = true;
                            break;
                        }
                    }
                    if(!isMovingPoint && !objInfoBeingEdited->obj->collides_with_cam_coords(drawP.world.drawData.cam.c, cMouseCircle))
                        clickedAway = true;
                }

                for(HandleData& h : pointHandles) {
                    if(SCollision::collide(mouseCircle, SCollision::Circle<float>(drawP.world.drawData.cam.c.to_space(objInfoBeingEdited->obj->coords.from_space(h.coordMatrix * (*h.p))), drawP.drag_point_radius()))) {
                        pointDragging = &h;
                        isMovingPoint = true;
                        break;
                    }
                }
                if(!isMovingPoint && !objInfoBeingEdited->obj->collides_with_cam_coords(drawP.world.drawData.cam.c, cMouseCircle))
                    clickedAway = true;

                if(clickedAway)
                    switch_tool(get_type());

                if(objInfoBeingEdited)
                    compEditTool->input_mouse_button_on_canvas_callback(objInfoBeingEdited, button, pointDragging);
            }
        }
        else {
            if(objInfoBeingEdited)
                compEditTool->input_mouse_button_on_canvas_callback(objInfoBeingEdited, button, pointDragging);
            if(pointDragging)
                pointDragging = nullptr;
        }
    }
}

void EditTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(objInfoBeingEdited) {
        if(drawP.controls.leftClickHeld && pointDragging) {
            Vector2f newPos = pointDragging->coordMatrix.inverse() * objInfoBeingEdited->obj->coords.get_mouse_pos(drawP.world);
            if(newPos != *pointDragging->p) {
                if(pointDragging->min)
                    newPos = cwise_vec_max((*pointDragging->min + Vector2f{pointDragging->minimumDistanceBetweenBoundsAndPoint, pointDragging->minimumDistanceBetweenBoundsAndPoint}).eval(), newPos);
                if(pointDragging->max)
                    newPos = cwise_vec_min((*pointDragging->max - Vector2f{pointDragging->minimumDistanceBetweenBoundsAndPoint, pointDragging->minimumDistanceBetweenBoundsAndPoint}).eval(), newPos);
                *pointDragging->p = newPos;
                objInfoBeingEdited->obj->commit_update(drawP);
            }
        }
        compEditTool->input_mouse_motion_callback(objInfoBeingEdited, motion, pointDragging);
    }
    drawP.selection.input_mouse_motion_callback_modify_selection(motion);
}

void EditTool::right_click_popup_gui(Vector2f popupPos) {
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

        if(undoAfterEditDone) {
            class EditCanvasComponentWorldUndoAction : public WorldUndoAction {
                public:
                    EditCanvasComponentWorldUndoAction(std::unique_ptr<CanvasComponent> initData, WorldUndoManager::UndoObjectID initUndoID):
                        data(std::move(initData)),
                        undoID(initUndoID)
                    {}
                    std::string get_name() const override {
                        return "Edit Canvas Component";
                    }
                    bool undo(WorldUndoManager& undoMan) override {
                        return undo_redo(undoMan);
                    }
                    bool redo(WorldUndoManager& undoMan) override {
                        return undo_redo(undoMan);
                    }
                    bool undo_redo(WorldUndoManager& undoMan) {
                        std::optional<NetworkingObjects::NetObjID> toEditID = undoMan.get_netid_from_undoid(undoID);
                        if(!toEditID.has_value())
                            return false;
                        auto objPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<CanvasComponentContainer>(toEditID.value());
                        std::unique_ptr<CanvasComponent> newData = objPtr->get_comp().get_data_copy();
                        objPtr->get_comp().set_data_from(*data);
                        data = std::move(newData);
                        objPtr->commit_update(undoMan.world.drawProg);
                        objPtr->send_comp_update(undoMan.world.drawProg, true);
                        return true;
                    }
                    ~EditCanvasComponentWorldUndoAction() {}
    
                    std::unique_ptr<CanvasComponent> data;
                    WorldUndoManager::UndoObjectID undoID;
            };
    
            drawP.world.undo.push(std::make_unique<EditCanvasComponentWorldUndoAction>(std::move(oldData), drawP.world.undo.get_undoid_from_netid(objInfoBeingEdited->obj.get_net_id())));
        }

        oldData = nullptr;
        objInfoBeingEdited = nullptr;
    }
    pointHandles.clear();
    pointDragging = nullptr;

    if(!drawP.is_selection_allowing_tool(newTool))
        drawP.selection.deselect_all();
}

void EditTool::edit_start(CanvasComponentContainer::ObjInfo* comp, bool initUndoAfterEditDone) {
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
        case CanvasComponentType::BRUSHSTROKE: {
            compEditTool = std::make_unique<BrushEditTool>(drawP);
            break;
        }
        default: {
            isEditing = false;
            break;
        }
    }
    if(isEditing) {
        objInfoBeingEdited = comp;
        oldData = comp->obj->get_comp().get_data_copy();
        undoAfterEditDone = initUndoAfterEditDone;
        compEditTool->edit_start(*this, objInfoBeingEdited, prevData);
    }
}

bool EditTool::is_editable(CanvasComponentContainer::ObjInfo* comp) {
    return true; // Brush strokes used to not be editable, but now they are so this always returns true
}

void EditTool::tool_update() {
    if(objInfoBeingEdited) {
        objInfoBeingEdited->obj->send_comp_update(drawP, false);
        bool shouldNotReset = compEditTool->edit_update(objInfoBeingEdited);
        if(!shouldNotReset)
            switch_tool(get_type());
    }
}

bool EditTool::prevent_undo_or_redo() {
    return objInfoBeingEdited || drawP.selection.is_something_selected();
}

void EditTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(objInfoBeingEdited) {
        for(HandleData& h : pointHandles)
            drawP.draw_drag_circle(canvas, drawData.cam.c.to_space((objInfoBeingEdited->obj->coords.from_space(h.coordMatrix * *h.p))), {0.1f, 0.9f, 0.9f, 1.0f}, drawData);
    }
}

EditTool::~EditTool() { }
