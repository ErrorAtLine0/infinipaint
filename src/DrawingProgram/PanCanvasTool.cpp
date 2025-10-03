#include "PanCanvasTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include <Helpers/Logger.hpp>

PanCanvasTool::PanCanvasTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{
}

DrawingProgramToolType PanCanvasTool::get_type() {
    return DrawingProgramToolType::PAN;
}

void PanCanvasTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("Pan canvas tool");
    t.gui.text_label_centered("Pan tool");
    t.gui.pop_id();
}

void PanCanvasTool::tool_update() {
}

bool PanCanvasTool::prevent_undo_or_redo() {
    return false;
}

void PanCanvasTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}

void PanCanvasTool::switch_tool(DrawingProgramToolType newTool) {
    if(!drawP.is_selection_allowing_tool(newTool))
        drawP.selection.deselect_all();
}
