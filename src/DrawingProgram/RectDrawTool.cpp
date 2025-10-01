#include "RectDrawTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include <cereal/types/vector.hpp>

RectDrawTool::RectDrawTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType RectDrawTool::get_type() {
    return DrawingProgramToolType::RECTANGLE;
}

void RectDrawTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("rect draw tool");
    t.gui.text_label_centered("Draw Rectangle");
    t.gui.slider_scalar_field("relradiuswidth", "Corner Radius", &controls.relativeRadiusWidth, 0.0f, 40.0f);
    if(t.gui.radio_button_field("fillonly", "Fill only", controls.fillStrokeMode == 0)) controls.fillStrokeMode = 0;
    if(t.gui.radio_button_field("outlineonly", "Outline only", controls.fillStrokeMode == 1)) controls.fillStrokeMode = 1;
    if(t.gui.radio_button_field("filloutline", "Fill and Outline", controls.fillStrokeMode == 2)) controls.fillStrokeMode = 2;
    if(controls.fillStrokeMode == 1 || controls.fillStrokeMode == 2)
        t.gui.slider_scalar_field("relstrokewidth", "Outline Size", &drawP.controls.relativeWidth, 3.0f, 40.0f);
    t.gui.pop_id();
}

void RectDrawTool::switch_tool(DrawingProgramToolType newTool) {
    commit_rectangle();
    controls.drawStage = 0;
}

void RectDrawTool::tool_update() {
    switch(controls.drawStage) {
        case 0: {
            if(drawP.controls.leftClick) {
                controls.startAt = drawP.world.main.input.mouse.pos;
                controls.intermediateItem = std::make_shared<DrawRectangle>();
                controls.intermediateItem->d.strokeColor = drawP.controls.foregroundColor;
                controls.intermediateItem->d.fillColor = drawP.controls.backgroundColor;
                controls.intermediateItem->d.cornerRadius = controls.relativeRadiusWidth;
                controls.intermediateItem->d.strokeWidth = drawP.controls.relativeWidth;
                controls.intermediateItem->d.p1 = controls.startAt;
                controls.intermediateItem->d.p2 = controls.startAt;
                controls.intermediateItem->d.p2 = ensure_points_have_distance(controls.intermediateItem->d.p1, controls.intermediateItem->d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);
                controls.intermediateItem->d.fillStrokeMode = static_cast<uint8_t>(controls.fillStrokeMode);
                controls.intermediateItem->coords = drawP.world.drawData.cam.c;
                controls.intermediateItem->lastUpdateTime = std::chrono::steady_clock::now();
                uint64_t placement = drawP.components.client_list().size();
                auto objAdd = drawP.components.client_insert(placement, controls.intermediateItem);
                controls.intermediateItem->commit_update(drawP);
                controls.intermediateItem->client_send_place(drawP);
                drawP.add_undo_place_component(objAdd);
                controls.drawStage = 1;
            }
            break;
        }
        case 1: {
            if(drawP.controls.leftClickHeld) {
                controls.intermediateItem->lastUpdateTime = std::chrono::steady_clock::now();
                Vector2f newPos = controls.intermediateItem->coords.get_mouse_pos(drawP.world);
                if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held) {
                    float height = std::fabs(controls.startAt.y() - newPos.y());
                    newPos.x() = controls.startAt.x() + (((newPos.x() - controls.startAt.x()) < 0.0f ? -1.0f : 1.0f) * height);
                }
                controls.intermediateItem->d.p1 = cwise_vec_min(controls.startAt, newPos);
                controls.intermediateItem->d.p2 = cwise_vec_max(controls.startAt, newPos);
                controls.intermediateItem->d.p2 = ensure_points_have_distance(controls.intermediateItem->d.p1, controls.intermediateItem->d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);
                controls.intermediateItem->client_send_update(drawP, false);
                controls.intermediateItem->commit_update(drawP);
            }
            else {
                commit_rectangle();
                controls.drawStage = 0;
            }
            break;
        }
    }
}

bool RectDrawTool::prevent_undo_or_redo() {
    return controls.intermediateItem != nullptr;
}

void RectDrawTool::commit_rectangle() {
    if(controls.intermediateItem && controls.intermediateItem->collabListInfo.lock()) {
        controls.intermediateItem->client_send_update(drawP, true);
        controls.intermediateItem->commit_update(drawP);
    }
    controls.intermediateItem = nullptr; 
}

void RectDrawTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}
