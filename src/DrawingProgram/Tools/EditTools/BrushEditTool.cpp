#include "BrushEditTool.hpp"
#include "../../DrawingProgram.hpp"
#include "../../../World.hpp"
#include "../../../MainProgram.hpp"
#include "../EditTool.hpp"

#include "../../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/LayoutHelpers.hpp"

BrushEditTool::BrushEditTool(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp):
    DrawingProgramEditToolBase(initDrawP, initComp)
{}

void BrushEditTool::edit_gui() {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& a = static_cast<BrushStrokeCanvasComponent&>(comp->obj->get_comp());
    auto& gui = drawP.world.main.g.gui;
    Toolbar& t = drawP.world.main.toolbar;
    auto commit_update_func = [&] { comp->obj->commit_update(drawP); };

    gui.new_id("edit tool brush", [&] {
        text_label_centered(gui, "Edit Brush Stroke");
        left_to_right_line_layout(gui, [&]() {
            t.color_button_right("Outline Color", &a.d.color, { .onChange = commit_update_func });
            text_label(gui, "Outline Color");
        });
    });
}

void BrushEditTool::edit_start(EditTool& editTool, std::any& prevData) {
    auto& a = static_cast<BrushStrokeCanvasComponent&>(comp->obj->get_comp());
    oldColor = a.d.color;
}

void BrushEditTool::commit_edit_updates(std::any& prevData) {
}

bool BrushEditTool::edit_update() {
    return true;
}
