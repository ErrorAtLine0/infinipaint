#include "BrushEditTool.hpp"
#include "../../DrawingProgram.hpp"
#include "../../../World.hpp"
#include "../../../MainProgram.hpp"
#include "../EditTool.hpp"

BrushEditTool::BrushEditTool(DrawingProgram& initDrawP):
    DrawingProgramEditToolBase(initDrawP)
{}

bool BrushEditTool::edit_gui(CanvasComponentContainer::ObjInfo* comp) {
    auto& a = static_cast<BrushStrokeCanvasComponent&>(comp->obj->get_comp());
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("edit tool brush");
    t.gui.text_label_centered("Edit Brush Stroke");
    t.gui.left_to_right_line_layout([&]() {
        if(t.gui.color_button_big("Outline Color", &a.d.color, &a.d.color == t.colorRight))
            t.color_selector_right(&a.d.color == t.colorRight ? nullptr : &a.d.color);
        t.gui.text_label("Outline Color");
    });
    t.gui.pop_id();

    bool editHappened = a.d.color != oldColor;
    oldColor = a.d.color;
    return editHappened;
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
