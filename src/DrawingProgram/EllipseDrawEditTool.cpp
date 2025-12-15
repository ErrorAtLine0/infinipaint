#include "EllipseDrawEditTool.hpp"
#include "DrawingProgram.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include <cereal/types/vector.hpp>
#include <memory>
#include "EditTool.hpp"

EllipseDrawEditTool::EllipseDrawEditTool(DrawingProgram& initDrawP):
    DrawingProgramEditToolBase(initDrawP)
{}

bool EllipseDrawEditTool::edit_gui(const CanvasComponentContainer::ObjInfoSharedPtr& comp) {
    auto& a = static_cast<EllipseCanvasComponent&>(comp->obj->get_comp());

    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("edit tool ellipse");
    t.gui.text_label_centered("Edit Ellipse");
    if(t.gui.radio_button_field("fillonly", "Fill only", a.d.fillStrokeMode == 0)) a.d.fillStrokeMode = 0;
    if(t.gui.radio_button_field("outlineonly", "Outline only", a.d.fillStrokeMode == 1)) a.d.fillStrokeMode = 1;
    if(t.gui.radio_button_field("filloutline", "Fill and Outline", a.d.fillStrokeMode == 2)) a.d.fillStrokeMode = 2;
    if(a.d.fillStrokeMode == 0 || a.d.fillStrokeMode == 2) {
        t.gui.left_to_right_line_layout([&]() {
            if(t.gui.color_button_big("Fill Color", &a.d.fillColor, &a.d.fillColor == t.colorRight))
                t.color_selector_right(&a.d.fillColor == t.colorRight ? nullptr : &a.d.fillColor);
            t.gui.text_label("Fill Color");
        });
    }
    if(a.d.fillStrokeMode == 1 || a.d.fillStrokeMode == 2) {
        t.gui.slider_scalar_field("relstrokewidth", "Outline Size", &a.d.strokeWidth, 3.0f, 40.0f);
        t.gui.left_to_right_line_layout([&]() {
            if(t.gui.color_button_big("Outline Color", &a.d.strokeColor, &a.d.strokeColor == t.colorRight))
                t.color_selector_right(&a.d.strokeColor == t.colorRight ? nullptr : &a.d.strokeColor);
            t.gui.text_label("Outline Color");
        });
    }
    t.gui.pop_id();
 
    bool editHappened = (!oldData.has_value()) || (a.d != oldData);
    oldData = a.d;
    return editHappened;   
}

void EllipseDrawEditTool::edit_start(EditTool& editTool, const CanvasComponentContainer::ObjInfoSharedPtr& comp, std::any& prevData) {
    auto& a = static_cast<EllipseCanvasComponent&>(comp->obj->get_comp());

    prevData = a.d;
    editTool.add_point_handle({&a.d.p1, nullptr, &a.d.p2});
    editTool.add_point_handle({&a.d.p2, &a.d.p1, nullptr});
}

void EllipseDrawEditTool::commit_edit_updates(const CanvasComponentContainer::ObjInfoSharedPtr& comp, std::any& prevData) {
}

bool EllipseDrawEditTool::edit_update(const CanvasComponentContainer::ObjInfoSharedPtr& comp) {
    return true;
}
