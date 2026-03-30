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

    Toolbar& t = drawP.world.main.toolbar;
    t.gui.new_id("edit tool brush", [&] {
        text_label_centered(t.gui, "Edit Brush Stroke");
        left_to_right_line_layout(t.gui, [&]() {
            //if(t.gui.color_button_big("Outline Color", &a.d.color, &a.d.color == t.colorRight))
            //    t.color_selector_right(&a.d.color == t.colorRight ? nullptr : &a.d.color);
            text_label(t.gui, "Outline Color");
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
