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

#include "FillTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "../../CoordSpaceHelper.hpp"
#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../CanvasComponents/MeshCanvasComponent.hpp"

FillTool::FillTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType FillTool::get_type() {
    return DrawingProgramToolType::FILL;
}

void FillTool::gui_toolbox(Toolbar& t) {
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("fill tool", [&] {
        GUIStuff::ElementHelpers::text_label_centered(gui, "Fill");
    });
}

void FillTool::gui_phone_toolbox(PhoneDrawingProgramScreen& t) {
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("fill tool", [&] {
    });
}

void FillTool::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    drawP.selection_action_menu(popupPos);
}

void FillTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        if(button.down && drawP.layerMan.is_a_layer_being_edited() && !drawP.world.main.g.gui.cursor_obstructed()) {
            auto& toolConfig = drawP.world.main.toolConfig;

            CanvasComponentContainer* newContainer = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::MESH);
            MeshCanvasComponent& newMesh = static_cast<MeshCanvasComponent&>(newContainer->get_comp());

            newMesh.d.color = toolConfig.globalConf.foregroundColor;
            newMesh.d.points.fill.emplace_back();
            newMesh.d.points.fill.back().emplace_back(button.pos + Vector2f{0.0f, 100.0f});
            newMesh.d.points.fill.back().emplace_back(button.pos + Vector2f{-100.0f, -100.0f});
            newMesh.d.points.fill.back().emplace_back(button.pos + Vector2f{100.0f, -100.0f});
            newContainer->coords = drawP.world.drawData.cam.c;

            objInfoBeingEdited = drawP.layerMan.add_component_to_layer_being_edited(newContainer);

            objInfoBeingEdited->obj->commit_update(drawP);
            objInfoBeingEdited->obj->send_comp_update(drawP, true);

            drawP.layerMan.add_undo_place_component(objInfoBeingEdited);

            objInfoBeingEdited = nullptr;
        }
    }
}

Vector4f* FillTool::color_picker_color(Vector4f* oldColor) {
    return nullptr;
}

void FillTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
}

void FillTool::switch_tool(DrawingProgramToolType newTool) {
}

void FillTool::tool_update() {
}

bool FillTool::prevent_undo_or_redo() {
    return false;
}

void FillTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}
