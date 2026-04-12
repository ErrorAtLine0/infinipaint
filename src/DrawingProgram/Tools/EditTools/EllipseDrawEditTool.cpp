#include "EllipseDrawEditTool.hpp"
#include "../../DrawingProgram.hpp"
#include "../../../World.hpp"
#include "../../../MainProgram.hpp"
#include "../../../DrawData.hpp"
#include <cereal/types/vector.hpp>
#include "../EditTool.hpp"

#include "../../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/NumberSliderHelpers.hpp"

EllipseDrawEditTool::EllipseDrawEditTool(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp):
    DrawingProgramEditToolBase(initDrawP, initComp)
{}

void EllipseDrawEditTool::edit_gui(Toolbar& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& a = static_cast<EllipseCanvasComponent&>(comp->obj->get_comp());
    auto& gui = drawP.world.main.g.gui;
    auto commit_update_func = [&] { comp->obj->commit_update(drawP); };
    auto commit_update_and_layout_func = [&] {
        comp->obj->commit_update(drawP);
        drawP.world.main.g.gui.set_to_layout();
    };

    gui.new_id("edit tool ellipse", [&] {
        text_label_centered(gui, "Edit Ellipse");
        radio_button_selector(gui, "Fill selector", &a.d.fillStrokeMode, {
            {"Fill only", 0},
            {"Outline only", 1},
            {"Fill and outline", 2}
        }, commit_update_and_layout_func);
        if(a.d.fillStrokeMode == 0 || a.d.fillStrokeMode == 2) {
            left_to_right_line_layout(gui, [&] {
                t.color_button_right("Fill color button", &a.d.fillColor, { .onChange = commit_update_func });
                text_label(gui, "Fill Color");
            });
        }
        if(a.d.fillStrokeMode == 1 || a.d.fillStrokeMode == 2) {
            slider_scalar_field(gui, "relstrokewidth", "Outline Size", &a.d.strokeWidth, 3.0f, 40.0f, {.onEdit = commit_update_func});
            left_to_right_line_layout(gui, [&] {
                t.color_button_right("Outline color button", &a.d.strokeColor, { .onChange = commit_update_func });
                text_label(gui, "Outline Color");
            });
        }
    });
}

void EllipseDrawEditTool::edit_start(EditTool& editTool, std::any& prevData) {
    auto& a = static_cast<EllipseCanvasComponent&>(comp->obj->get_comp());

    prevData = a.d;
    editTool.add_point_handle({&a.d.p1, nullptr, &a.d.p2});
    editTool.add_point_handle({&a.d.p2, &a.d.p1, nullptr});
}

void EllipseDrawEditTool::commit_edit_updates(std::any& prevData) {
}

bool EllipseDrawEditTool::edit_update() {
    return true;
}
