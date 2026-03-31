#include "ZoomCanvasTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include <Helpers/Logger.hpp>

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"

ZoomCanvasTool::ZoomCanvasTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{
}

DrawingProgramToolType ZoomCanvasTool::get_type() {
    return DrawingProgramToolType::ZOOM;
}

void ZoomCanvasTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("Zoom canvas tool", [&] {
        GUIStuff::ElementHelpers::text_label_centered(gui, "Zoom tool");
    });
}

void ZoomCanvasTool::right_click_popup_gui(Vector2f popupPos) {
    drawP.selection_action_menu(popupPos);
}

void ZoomCanvasTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
}

void ZoomCanvasTool::tool_update() {
}

bool ZoomCanvasTool::prevent_undo_or_redo() {
    return false;
}

void ZoomCanvasTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}

void ZoomCanvasTool::switch_tool(DrawingProgramToolType newTool) {
    //if(!drawP.is_selection_allowing_tool(newTool))
    //    drawP.selection.deselect_all();
}
