#include "RectDrawTool.hpp"
#include "DrawingProgram.hpp"
#include "../Server/CommandList.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "Helpers/SCollision.hpp"
#include "Helpers/Serializers.hpp"
#include "../SharedTypes.hpp"
#include <cereal/types/vector.hpp>

RectDrawTool::RectDrawTool(DrawingProgram& initDrawP):
    drawP(initDrawP)
{}

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

bool RectDrawTool::edit_gui(const std::shared_ptr<DrawRectangle>& a) {
    DrawRectangle::Data oldData = a->d;
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("edit tool rectangle");
    t.gui.text_label_centered("Edit Rectangle");
    t.gui.slider_scalar_field("relradiuswidth", "Corner Radius", &a->d.cornerRadius, 0.0f, 40.0f);
    if(t.gui.radio_button_field("fillonly", "Fill only", a->d.fillStrokeMode == 0)) a->d.fillStrokeMode = 0;
    if(t.gui.radio_button_field("outlineonly", "Outline only", a->d.fillStrokeMode == 1)) a->d.fillStrokeMode = 1;
    if(t.gui.radio_button_field("filloutline", "Fill and Outline", a->d.fillStrokeMode == 2)) a->d.fillStrokeMode = 2;
    if(a->d.fillStrokeMode == 0 || a->d.fillStrokeMode == 2) {
        t.gui.left_to_right_line_layout([&]() {
            CLAY({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(40), .height = CLAY_SIZING_FIXED(40)}}}) {
                if(t.gui.color_button("Fill Color", &a->d.fillColor, &a->d.fillColor == t.colorRight))
                    t.color_selector_right(&a->d.fillColor == t.colorRight ? nullptr : &a->d.fillColor);
            }
            t.gui.text_label("Fill Color");
        });
    }
    if(a->d.fillStrokeMode == 1 || a->d.fillStrokeMode == 2) {
        t.gui.slider_scalar_field("relstrokewidth", "Outline Size", &a->d.strokeWidth, 3.0f, 40.0f);
        t.gui.left_to_right_line_layout([&]() {
            CLAY({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(40), .height = CLAY_SIZING_FIXED(40)}}}) {
                if(t.gui.color_button("Outline Color", &a->d.strokeColor, &a->d.strokeColor == t.colorRight))
                    t.color_selector_right(&a->d.strokeColor == t.colorRight ? nullptr : &a->d.strokeColor);
            }
            t.gui.text_label("Outline Color");
        });
    }
    t.gui.pop_id();
    return a->d != oldData;
}

void RectDrawTool::reset_tool() {
    commit_rectangle();
    controls.drawStage = 0;
}

void RectDrawTool::edit_start(const std::shared_ptr<DrawRectangle>& a, std::any& prevData) {
    prevData = a->d;
    drawP.editTool.add_point_handle({&a->d.p1, nullptr, &a->d.p2});
    drawP.editTool.add_point_handle({&a->d.p2, &a->d.p1, nullptr});
}

void RectDrawTool::commit_edit_updates(const std::shared_ptr<DrawRectangle>& a, std::any& prevData) {
    DrawRectangle::Data pData = std::any_cast<DrawRectangle::Data>(prevData);
    DrawRectangle::Data cData = a->d;
    drawP.world.undo.push(UndoManager::UndoRedoPair{
        [&, a, pData]() {
            a->d = pData;
            a->client_send_update_final(drawP);
            a->final_update(drawP);
            drawP.reset_tools();
            return true;
        },
        [&, a, cData]() {
            a->d = cData;
            a->client_send_update_final(drawP);
            a->final_update(drawP);
            drawP.reset_tools();
            return true;
        }
    });
}

bool RectDrawTool::edit_update(const std::shared_ptr<DrawRectangle>& a) {
    return true;
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
                controls.intermediateItem->d.fillStrokeMode = static_cast<uint8_t>(controls.fillStrokeMode);
                controls.intermediateItem->coords = drawP.world.drawData.cam.c;
                controls.intermediateItem->lastUpdateTime = std::chrono::steady_clock::now();
                uint64_t placement = drawP.components.client_list().size();
                auto objAdd = drawP.components.client_insert(placement, controls.intermediateItem);
                controls.intermediateItem->temp_update(drawP);
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
                if(drawP.world.main.input.key(InputManager::KEY_EQUAL_DIMENSIONS).held) {
                    float height = std::fabs(controls.startAt.y() - newPos.y());
                    newPos.x() = controls.startAt.x() + (((newPos.x() - controls.startAt.x()) < 0.0f ? -1.0f : 1.0f) * height);
                }
                controls.intermediateItem->d.p1 = cwise_vec_min(controls.startAt, newPos);
                controls.intermediateItem->d.p2 = cwise_vec_max(controls.startAt, newPos);
                controls.intermediateItem->client_send_update_temp(drawP);
                controls.intermediateItem->temp_update(drawP);
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
        controls.intermediateItem->client_send_update_final(drawP);
        controls.intermediateItem->final_update(drawP);
    }
    controls.intermediateItem = nullptr; 
}

void RectDrawTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}
