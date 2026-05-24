/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "EditTool.hpp"
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
#include "EditTools/MeshEditTool.hpp"
#include "../EditCanvasComponentWorldUndoAction.hpp"

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"

EditTool::EditTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType EditTool::get_type() {
    return DrawingProgramToolType::EDIT;
}

void EditTool::gui_toolbox(Toolbar& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;

    if(objInfoBeingEdited)
        compEditTool->edit_gui(t);
    else {
        gui.new_id("edit tool", [&] {
            text_label_centered(gui, "Edit");
        });
    }
}

void EditTool::gui_phone_toolbox(PhoneDrawingProgramScreen& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;

    if(objInfoBeingEdited)
        compEditTool->gui_phone_toolbox(t);
    else {
        gui.new_id("edit tool", [&] {
            text_label_centered(gui, "Double tap object to edit");
        });
    }
}

void EditTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    if(erasedComp == objInfoBeingEdited)
        switch_tool(get_type());
}

void EditTool::input_paste_callback(const CustomEvents::PasteEvent& paste) {
    if(objInfoBeingEdited)
        compEditTool->input_paste_callback(paste);
}

void EditTool::input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) {
    if(objInfoBeingEdited)
        compEditTool->input_android_text_box_input_callback(textboxInput);
}

void EditTool::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {
    if(objInfoBeingEdited)
        compEditTool->input_text_key_callback(key);
}

void EditTool::input_text_callback(const InputManager::TextCallbackArgs& text) {
    if(objInfoBeingEdited)
        compEditTool->input_text_callback(text);
}

void EditTool::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    drawP.selection.input_key_callback_modify_selection(key);
}

void EditTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    drawP.selection.input_mouse_button_on_canvas_callback_modify_selection(button);
    if(button.button == InputManager::MouseButton::LEFT) {
        if(button.down && !drawP.world.main.g.gui.cursor_obstructed()) {
            if(!objInfoBeingEdited) {
                WorldVec mouseWorldPos = drawP.world.drawData.cam.c.from_space(button.pos);

                SkPath camMouseAABB = SkPath::Rect(SkRect::MakeLTRB(drawP.world.main.input.mouse.pos.x() - 0.5f, drawP.world.main.input.mouse.pos.y() - 0.5f, drawP.world.main.input.mouse.pos.x() + 0.5f, drawP.world.main.input.mouse.pos.y() + 0.5f));

                bool modifySelection = !drawP.selection.is_being_transformed();
                if(button.clicks >= 2 && !drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held && !drawP.world.main.input.key(InputManager::KEY_GENERIC_LALT).held) {
                    CanvasComponentContainer::ObjInfo* selectedObjectToEdit = drawP.selection.get_front_object_colliding_with_in_editing_layer(camMouseAABB);

                    if(selectedObjectToEdit && is_editable(selectedObjectToEdit)) {
                        drawP.selection.deselect_all();
                        edit_start(selectedObjectToEdit);
                        modifySelection = false;
                    }
                }
                if(modifySelection) {
                    if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held)
                        drawP.selection.add_from_cam_coord_collider_to_selection(camMouseAABB, DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED, true);
                    else if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LALT).held)
                        drawP.selection.remove_from_cam_coord_collider_to_selection(camMouseAABB, DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED, true);
                    else {
                        drawP.selection.deselect_all();
                        drawP.selection.add_from_cam_coord_collider_to_selection(camMouseAABB, DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED, true);
                    }
                }
            }
            else {
                bool isMovingPoint = false;
                bool clickedAway = false;

                if(!pointDragging) {
                    for(HandleData& h : pointHandles) {
                        if(SCollision::collide(button.pos, SCollision::Circle<float>(drawP.world.drawData.cam.c.to_space(objInfoBeingEdited->obj->coords.from_space(h.coordMatrix * (*h.p))), drawP.drag_point_radius()))) {
                            pointDragging = &h;
                            isMovingPoint = true;
                        }
                    }
                    if(!isMovingPoint && !objInfoBeingEdited->obj->collides_with_point(drawP.world.drawData.cam.c, button.pos))
                        clickedAway = true;
                }

                for(HandleData& h : pointHandles) {
                    if(SCollision::collide(button.pos, SCollision::Circle<float>(drawP.world.drawData.cam.c.to_space(objInfoBeingEdited->obj->coords.from_space(h.coordMatrix * (*h.p))), drawP.drag_point_radius()))) {
                        pointDragging = &h;
                        isMovingPoint = true;
                        break;
                    }
                }
                if(!isMovingPoint && !objInfoBeingEdited->obj->collides_with_point(drawP.world.drawData.cam.c, button.pos))
                    clickedAway = true;

                if(clickedAway)
                    switch_tool(get_type());

                if(objInfoBeingEdited)
                    compEditTool->input_mouse_button_on_canvas_callback(button, pointDragging);
            }
        }
        else {
            if(objInfoBeingEdited)
                compEditTool->input_mouse_button_on_canvas_callback(button, pointDragging);
            if(pointDragging)
                pointDragging = nullptr;
        }
        drawP.world.main.g.gui.set_to_layout();
    }
}

void EditTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(objInfoBeingEdited) {
        if(drawP.controls.leftClickHeld && pointDragging) {
            Vector2f newPos = pointDragging->coordMatrix.inverse() * objInfoBeingEdited->obj->coords.get_mouse_pos(drawP.world);
            if(newPos != *pointDragging->p) {
                if(pointDragging->min)
                    newPos = cwise_vec_max((*pointDragging->min + Vector2f{pointDragging->minimumDistanceBetweenMinAndPoint, pointDragging->minimumDistanceBetweenMinAndPoint}).eval(), newPos);
                if(pointDragging->max)
                    newPos = cwise_vec_min((*pointDragging->max - Vector2f{pointDragging->minimumDistanceBetweenMaxAndPoint, pointDragging->minimumDistanceBetweenMaxAndPoint}).eval(), newPos);
                *pointDragging->p = newPos;
                compEditTool->commitUpdate = true;
            }
        }
        compEditTool->input_mouse_motion_callback(motion, pointDragging);
    }
    drawP.selection.input_mouse_motion_callback_modify_selection(motion);
}

std::optional<InputManager::TextBoxStartInfo> EditTool::get_text_box_start_info() {
    if(objInfoBeingEdited)
        return compEditTool->get_text_box_start_info();
    return std::nullopt;
}

void EditTool::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    if(objInfoBeingEdited)
        return compEditTool->right_click_popup_gui(t, popupPos);
    else
        return drawP.selection_action_menu(popupPos);
}

void EditTool::add_point_handle(const HandleData& handle) {
    pointHandles.emplace_back(handle);
}

void EditTool::switch_tool(DrawingProgramToolType newTool) {
    if(objInfoBeingEdited) {
        compEditTool->commit_edit_updates(prevData);
        objInfoBeingEdited->obj->commit_update(drawP);
        objInfoBeingEdited->obj->send_comp_update(drawP, true);

        if(undoAfterEditDone)
            drawP.world.undo.push(std::make_unique<EditCanvasComponentWorldUndoAction>(std::move(oldData), drawP.world.undo.get_undoid_from_netid(objInfoBeingEdited->obj.get_net_id())));

        oldData = nullptr;
        objInfoBeingEdited = nullptr;
        drawP.world.main.g.gui.set_to_layout();
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
            compEditTool = std::make_unique<TextBoxEditTool>(drawP, comp);
            break;
        }
        case CanvasComponentType::ELLIPSE: {
            compEditTool = std::make_unique<EllipseDrawEditTool>(drawP, comp);
            break;
        }
        case CanvasComponentType::RECTANGLE: {
            compEditTool = std::make_unique<RectDrawEditTool>(drawP, comp);
            break;
        }
        case CanvasComponentType::IMAGE: {
            compEditTool = std::make_unique<ImageEditTool>(drawP, comp);
            break;
        }
        case CanvasComponentType::MESH: {
            compEditTool = std::make_unique<MeshEditTool>(drawP, comp);
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
        compEditTool->edit_start(*this, prevData);
        drawP.world.main.g.gui.set_to_layout();
    }
}

bool EditTool::is_editable(CanvasComponentContainer::ObjInfo* comp) {
    return true; // Brush strokes used to not be editable, but now they are so this always returns true
}

void EditTool::tool_update() {
    if(objInfoBeingEdited) {
        compEditTool->edit_update();
        if(compEditTool->commitUpdate) {
            objInfoBeingEdited->obj->commit_update(drawP);
            objInfoBeingEdited->obj->send_comp_update(drawP, false);
            compEditTool->commitUpdate = false;
        }
    }
}

Vector4f* EditTool::color_picker_color(Vector4f* oldColor) {
    if(objInfoBeingEdited)
        return compEditTool->color_picker_color(oldColor);
    return nullptr;
}

bool EditTool::phone_gui_tool_specific_bottom_toolbar_exists() {
    if(objInfoBeingEdited)
        return compEditTool->phone_gui_tool_specific_bottom_toolbar_exists();
    return false;
}

void EditTool::phone_gui_tool_specific_bottom_toolbar(PhoneDrawingProgramScreen& t) {
    if(objInfoBeingEdited)
        return compEditTool->phone_gui_tool_specific_bottom_toolbar(t);
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
