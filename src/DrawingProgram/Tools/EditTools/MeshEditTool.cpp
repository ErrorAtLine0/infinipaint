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

#include "MeshEditTool.hpp"
#include "../../DrawingProgram.hpp"
#include "../../../World.hpp"
#include "../../../MainProgram.hpp"
#include "../EditTool.hpp"
#include "../../../CanvasComponents/MeshCanvasComponent.hpp"

#include "../../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/LayoutHelpers.hpp"

MeshEditTool::MeshEditTool(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp):
    DrawingProgramEditToolBase(initDrawP, initComp)
{}

void MeshEditTool::edit_gui(Toolbar& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& a = static_cast<MeshCanvasComponent&>(comp->obj->get_comp());
    auto& gui = drawP.world.main.g.gui;
    auto commit_update_func = [&] { commitUpdate = true; };

    gui.new_id("edit tool mesh", [&] {
        text_label_centered(gui, "Edit Mesh");
        left_to_right_line_layout(gui, [&]() {
            t.color_button_right("Mesh Color", &a.d.color, { .onChange = commit_update_func });
            text_label(gui, "Mesh Color");
        });
    });
}

void MeshEditTool::gui_phone_toolbox(PhoneDrawingProgramScreen& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& a = static_cast<MeshCanvasComponent&>(comp->obj->get_comp());
    auto& gui = drawP.world.main.g.gui;
    auto commit_update_func = [&] { commitUpdate = true; };

    gui.new_id("edit tool mesh", [&] {
        left_to_right_line_layout(gui, [&]() {
            t.color_selector_button("Mesh Color", &a.d.color, { .onChange = commit_update_func });
            text_label(gui, "Mesh Color");
        });
    });
}

Vector4f* MeshEditTool::color_picker_color(Vector4f* oldColor) {
    auto& a = static_cast<MeshCanvasComponent&>(comp->obj->get_comp());
    if(oldColor == &a.d.color)
        return oldColor;
    return nullptr;
}

void MeshEditTool::edit_start(EditTool& editTool, std::any& prevData) {
    auto& a = static_cast<MeshCanvasComponent&>(comp->obj->get_comp());
    oldColor = a.d.color;
}

void MeshEditTool::commit_edit_updates(std::any& prevData) {
}

void MeshEditTool::edit_update() {
}
