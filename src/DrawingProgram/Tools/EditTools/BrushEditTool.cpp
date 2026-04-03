#include "BrushEditTool.hpp"
#include "../../DrawingProgram.hpp"
#include "../../../World.hpp"
#include "../../../MainProgram.hpp"
#include "../EditTool.hpp"

#include "../../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/LayoutHelpers.hpp"

BrushEditTool::BrushEditTool(DrawingProgram& initDrawP):
    DrawingProgramEditToolBase(initDrawP)
{}

void BrushEditTool::edit_gui(CanvasComponentContainer::ObjInfo* comp) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& a = static_cast<BrushStrokeCanvasComponent&>(comp->obj->get_comp());
    auto& gui = drawP.world.main.g.gui;
    Toolbar& t = drawP.world.main.toolbar;
    auto commit_update_func = [&, comp] { comp->obj->commit_update(drawP); };

    gui.new_id("edit tool brush", [&] {
        text_label_centered(gui, "Edit Brush Stroke");
        left_to_right_line_layout(gui, [&]() {
            t.color_button_right("Outline Color", &a.d.color, commit_update_func);
            text_label(gui, "Outline Color");
        });
    });
}

void BrushEditTool::edit_start(EditTool& editTool, CanvasComponentContainer::ObjInfo* comp, std::any& prevData) {
    auto& a = static_cast<BrushStrokeCanvasComponent&>(comp->obj->get_comp());
    oldColor = a.d.color;
}

void BrushEditTool::commit_edit_updates(CanvasComponentContainer::ObjInfo* comp, std::any& prevData) {
}

bool BrushEditTool::edit_update(CanvasComponentContainer::ObjInfo* comp) {
    return true;
}
