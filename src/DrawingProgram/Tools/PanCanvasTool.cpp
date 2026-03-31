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

void PanCanvasTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("Pan canvas tool", [&] {
        GUIStuff::ElementHelpers::text_label_centered(gui, "Pan tool");
    });
}

void PanCanvasTool::right_click_popup_gui(Vector2f popupPos) {
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
