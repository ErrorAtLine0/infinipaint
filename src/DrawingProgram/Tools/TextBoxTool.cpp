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

#include "TextBoxTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "Helpers/MathExtras.hpp"
#include <cereal/types/vector.hpp>
#include <memory>
#include "../../CanvasComponents/TextBoxCanvasComponent.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"
#include "EditTool.hpp"

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"

TextBoxTool::TextBoxTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType TextBoxTool::get_type() {
    return DrawingProgramToolType::TEXTBOX;
}

void TextBoxTool::gui_toolbox(Toolbar& t) {
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("textbox tool", [&] {
        GUIStuff::ElementHelpers::text_label_centered(gui, "Place textbox");
    });
}

void TextBoxTool::gui_phone_toolbox(PhoneDrawingProgramScreen& t) {
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("textbox tool", [&] {
        GUIStuff::ElementHelpers::text_label_centered(gui, "Place textbox");
    });
}

void TextBoxTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        if(button.down && drawP.layerMan.is_a_layer_being_edited() && !objInfoBeingEdited && !drawP.world.main.g.gui.cursor_obstructed()) {
            startAt = button.pos;
            endAt = startAt;

            CanvasComponentContainer* newContainer = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::TEXTBOX);
            TextBoxCanvasComponent& newTextBox = static_cast<TextBoxCanvasComponent&>(newContainer->get_comp());

            newTextBox.d.p1 = newTextBox.d.p2 = startAt;
            newTextBox.d.p2 = ensure_points_have_distance(newTextBox.d.p1, newTextBox.d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);
            newTextBox.d.editing = true;
            newContainer->coords = drawP.world.drawData.cam.c;
            if(drawP.world.main.g.gui.io.isTouchDevice)
                newContainer->coords.scale_about_double(drawP.world.drawData.cam.c.from_space(startAt), 1.0 / drawP.world.main.get_scale_and_density_factor_gui());
            objInfoBeingEdited = drawP.layerMan.add_component_to_layer_being_edited(newContainer);
        }
        else if(!button.down && objInfoBeingEdited) {
            make_sure_textbox_is_big();
            tool_update();
            auto editTool = std::make_unique<EditTool>(drawP);
            editTool->edit_start(objInfoBeingEdited, false);
            drawP.toolToSwitchToAfterUpdate = std::move(editTool);
        }
    }
}

void TextBoxTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        TextBoxCanvasComponent& textBox = static_cast<TextBoxCanvasComponent&>(containerPtr->get_comp());

        Vector2f newPos = containerPtr->coords.from_cam_space_to_this(drawP.world, motion.pos);
        if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held) {
            float height = std::fabs(startAt.y() - newPos.y());
            newPos.x() = startAt.x() + (((newPos.x() - startAt.x()) < 0.0f ? -1.0f : 1.0f) * height);
        }
        endAt = newPos;
        textBox.d.p1 = cwise_vec_min(endAt, startAt);
        textBox.d.p2 = cwise_vec_max(endAt, startAt);
        textBox.d.p2 = ensure_points_have_distance(textBox.d.p1, textBox.d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);
        commitUpdate = true;
    }
}

void TextBoxTool::make_sure_textbox_is_big() {
    if(objInfoBeingEdited) {
        const Vector2f TEXTBOX_MINIMUM_DIMENSIONS{200.0f, 100.0f};
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        TextBoxCanvasComponent& textBox = static_cast<TextBoxCanvasComponent&>(containerPtr->get_comp());
        if(vec_distance(textBox.d.p1, textBox.d.p2) < 20.0f)
            textBox.d.p2 = textBox.d.p1 + TEXTBOX_MINIMUM_DIMENSIONS;
    }
}

void TextBoxTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    if(objInfoBeingEdited == erasedComp) {
        objInfoBeingEdited = nullptr;
        commitUpdate = false;
    }
}

void TextBoxTool::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    t.paint_popup(popupPos);
}

void TextBoxTool::switch_tool(DrawingProgramToolType newTool) {
    commit();
}

void TextBoxTool::tool_update() {
    if(commitUpdate && objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        containerPtr->commit_update(drawP);
        containerPtr->send_comp_update(drawP, false);
        commitUpdate = false;
    }
}

bool TextBoxTool::prevent_undo_or_redo() {
    return objInfoBeingEdited;
}

void TextBoxTool::commit() {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        make_sure_textbox_is_big();
        containerPtr->commit_update(drawP);
        containerPtr->send_comp_update(drawP, true);
        commitUpdate = false;
        drawP.layerMan.add_undo_place_component(objInfoBeingEdited);
        objInfoBeingEdited = nullptr;
    }
}

void TextBoxTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}
