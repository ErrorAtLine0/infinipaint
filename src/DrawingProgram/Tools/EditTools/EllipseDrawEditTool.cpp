#include "EllipseDrawEditTool.hpp"
#include "../../DrawingProgram.hpp"
#include "../../../World.hpp"
#include "../../../MainProgram.hpp"
#include "../../../DrawData.hpp"
#include <cereal/types/vector.hpp>
#include <memory>
#include "../EditTool.hpp"

#include "../../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/NumberSliderHelpers.hpp"

EllipseDrawEditTool::EllipseDrawEditTool(DrawingProgram& initDrawP):
    DrawingProgramEditToolBase(initDrawP)
{}

void EllipseDrawEditTool::edit_gui(CanvasComponentContainer::ObjInfo* comp) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& a = static_cast<EllipseCanvasComponent&>(comp->obj->get_comp());

    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("edit tool ellipse");
    text_label_centered(t.gui, "Edit Ellipse");
    radio_button_selector(t.gui, "Fill selector", &a.d.fillStrokeMode, {
        {"Fill only", 0},
        {"Outline only", 1},
        {"Fill and outline", 2}
    });
    if(a.d.fillStrokeMode == 0 || a.d.fillStrokeMode == 2) {
        left_to_right_line_layout(t.gui, [&] {
            //if(t.gui.color_button_big("Fill Color", &a.d.fillColor, &a.d.fillColor == t.colorRight))
            //    t.color_selector_right(&a.d.fillColor == t.colorRight ? nullptr : &a.d.fillColor);
            text_label(t.gui, "Fill Color");
        });
    }
    if(a.d.fillStrokeMode == 1 || a.d.fillStrokeMode == 2) {
        slider_scalar_field(t.gui, "relstrokewidth", "Outline Size", &a.d.strokeWidth, 3.0f, 40.0f);
        left_to_right_line_layout(t.gui, [&] {
            //if(t.gui.color_button_big("Outline Color", &a.d.strokeColor, &a.d.strokeColor == t.colorRight))
            //    t.color_selector_right(&a.d.strokeColor == t.colorRight ? nullptr : &a.d.strokeColor);
            text_label(t.gui, "Outline Color");
        });
    }
    t.gui.pop_id();
}

void EllipseDrawEditTool::edit_start(EditTool& editTool, CanvasComponentContainer::ObjInfo* comp, std::any& prevData) {
    auto& a = static_cast<EllipseCanvasComponent&>(comp->obj->get_comp());

    prevData = a.d;
    editTool.add_point_handle({&a.d.p1, nullptr, &a.d.p2});
    editTool.add_point_handle({&a.d.p2, &a.d.p1, nullptr});
}

void EllipseDrawEditTool::commit_edit_updates(CanvasComponentContainer::ObjInfo* comp, std::any& prevData) {
}

bool EllipseDrawEditTool::edit_update(CanvasComponentContainer::ObjInfo* comp) {
    return true;
}
