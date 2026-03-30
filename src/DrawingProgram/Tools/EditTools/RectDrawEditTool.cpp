#include "RectDrawEditTool.hpp"
#include "../../DrawingProgram.hpp"
#include "../../../World.hpp"
#include "../../../MainProgram.hpp"
#include <memory>
#include "../EditTool.hpp"

#include "../../../GUIStuff/ElementHelpers/NumberSliderHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"

RectDrawEditTool::RectDrawEditTool(DrawingProgram& initDrawP):
    DrawingProgramEditToolBase(initDrawP)
{}

void RectDrawEditTool::edit_gui(CanvasComponentContainer::ObjInfo* comp) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& a = static_cast<RectangleCanvasComponent&>(comp->obj->get_comp());
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.new_id("edit tool rectangle", [&] {
        text_label_centered(t.gui, "Edit Rectangle");
        slider_scalar_field(t.gui, "relradiuswidth", "Corner Radius", &a.d.cornerRadius, 0.0f, 40.0f);
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
    });
}

void RectDrawEditTool::edit_start(EditTool& editTool, CanvasComponentContainer::ObjInfo* comp, std::any& prevData) {
    auto& a = static_cast<RectangleCanvasComponent&>(comp->obj->get_comp());
    prevData = a.d;
    editTool.add_point_handle({&a.d.p1, nullptr, &a.d.p2});
    editTool.add_point_handle({&a.d.p2, &a.d.p1, nullptr});
}

void RectDrawEditTool::commit_edit_updates(CanvasComponentContainer::ObjInfo* comp, std::any& prevData) {
}

bool RectDrawEditTool::edit_update(CanvasComponentContainer::ObjInfo* comp) {
    return true;
}
