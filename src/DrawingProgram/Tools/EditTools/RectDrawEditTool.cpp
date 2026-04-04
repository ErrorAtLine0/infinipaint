#include "RectDrawEditTool.hpp"
#include "../../DrawingProgram.hpp"
#include "../../../World.hpp"
#include "../../../MainProgram.hpp"
#include <memory>
#include "../EditTool.hpp"

#include "../../../GUIStuff/ElementHelpers/NumberSliderHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"

RectDrawEditTool::RectDrawEditTool(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp):
    DrawingProgramEditToolBase(initDrawP, initComp)
{}

void RectDrawEditTool::edit_gui() {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& a = static_cast<RectangleCanvasComponent&>(comp->obj->get_comp());
    auto commit_update_and_layout_func = [&] {
        comp->obj->commit_update(drawP);
        drawP.world.main.g.gui.set_to_layout();
    };
    auto commit_update_func = [&] { comp->obj->commit_update(drawP); };

    Toolbar& t = drawP.world.main.toolbar;
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("edit tool rectangle", [&] {
        text_label_centered(gui, "Edit Rectangle");
        slider_scalar_field(gui, "relradiuswidth", "Corner Radius", &a.d.cornerRadius, 0.0f, 40.0f, { .onEdit = commit_update_func });
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
            slider_scalar_field(gui, "relstrokewidth", "Outline Size", &a.d.strokeWidth, 3.0f, 40.0f, { .onEdit = commit_update_func });
            left_to_right_line_layout(gui, [&] {
                t.color_button_right("Outline color button", &a.d.strokeColor, { .onChange = commit_update_func });
                text_label(gui, "Outline Color");
            });
        }
    });
}

void RectDrawEditTool::edit_start(EditTool& editTool, std::any& prevData) {
    auto& a = static_cast<RectangleCanvasComponent&>(comp->obj->get_comp());
    prevData = a.d;
    editTool.add_point_handle({&a.d.p1, nullptr, &a.d.p2});
    editTool.add_point_handle({&a.d.p2, &a.d.p1, nullptr});
}

void RectDrawEditTool::commit_edit_updates(std::any& prevData) {
}

bool RectDrawEditTool::edit_update() {
    return true;
}
