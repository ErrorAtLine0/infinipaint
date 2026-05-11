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

#include "PanCanvasTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include <Helpers/Logger.hpp>

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"

PanCanvasTool::PanCanvasTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{
}

DrawingProgramToolType PanCanvasTool::get_type() {
    return DrawingProgramToolType::PAN;
}

void PanCanvasTool::gui_toolbox(Toolbar& t) {
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("Pan canvas tool", [&] {
        GUIStuff::ElementHelpers::text_label_centered(gui, "Pan tool");
    });
}

void PanCanvasTool::gui_phone_toolbox(PhoneDrawingProgramScreen& t) {
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("Pan canvas tool", [&] {
        GUIStuff::ElementHelpers::text_label_centered(gui, "Pan tool");
    });
}

void PanCanvasTool::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    drawP.selection_action_menu(popupPos);
}

void PanCanvasTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
}

void PanCanvasTool::tool_update() {
}

bool PanCanvasTool::prevent_undo_or_redo() {
    return false;
}

void PanCanvasTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}

void PanCanvasTool::switch_tool(DrawingProgramToolType newTool) {
    //if(!drawP.is_selection_allowing_tool(newTool))
    //    drawP.selection.deselect_all();
}
