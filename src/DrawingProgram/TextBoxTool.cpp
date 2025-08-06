#include "TextBoxTool.hpp"
#include <chrono>
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "../CollabTextBox/CollabTextBox.hpp"
#include <cereal/types/vector.hpp>
#include <memory>
#include "EditTool.hpp"

TextBoxTool::TextBoxTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType TextBoxTool::get_type() {
    return DrawingProgramToolType::TEXTBOX;
}

void TextBoxTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("textbox tool");
    t.gui.text_label_centered("Textbox");
    t.gui.slider_scalar_field("Text Relative Size", "Text Size", &controls.textRelativeSize, 5.0f, 100.0f);
    t.gui.pop_id();
}

void TextBoxTool::switch_tool(DrawingProgramToolType newTool) {
    commit();
    controls.drawStage = 0;
}

void TextBoxTool::tool_update() {
    switch(controls.drawStage) {
        case 0: {
            if(drawP.controls.leftClick) {
                controls.intermediateItem = std::make_shared<DrawTextBox>();
                controls.startAt = drawP.world.main.input.mouse.pos;
                controls.endAt = controls.startAt;
                controls.intermediateItem->d.p1 = controls.intermediateItem->d.p2 = controls.startAt;
                controls.intermediateItem->d.p2 = ensure_points_have_distance(controls.intermediateItem->d.p1, controls.intermediateItem->d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);
                controls.intermediateItem->d.textColor = drawP.controls.foregroundColor;
                controls.intermediateItem->d.textSize = controls.textRelativeSize;
                controls.intermediateItem->d.editing = true;
                controls.intermediateItem->coords = drawP.world.drawData.cam.c;
                controls.intermediateItem->lastUpdateTime = std::chrono::steady_clock::now();
                uint64_t placement = drawP.components.client_list().size();
                auto objAdd = drawP.components.client_insert(placement, controls.intermediateItem);
                controls.intermediateItem->client_send_place(drawP);
                drawP.add_undo_place_component(objAdd);
                controls.drawStage = 1;
            }
            break;
        }
        case 1: {
            Vector2f newPos = controls.intermediateItem->coords.get_mouse_pos(drawP.world);
            if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held) {
                float height = std::fabs(controls.startAt.y() - newPos.y());
                newPos.x() = controls.startAt.x() + (((newPos.x() - controls.startAt.x()) < 0.0f ? -1.0f : 1.0f) * height);
            }
            controls.endAt = newPos;
            controls.intermediateItem->d.p1 = cwise_vec_min(controls.endAt, controls.startAt);
            controls.intermediateItem->d.p2 = cwise_vec_max(controls.endAt, controls.startAt);
            controls.intermediateItem->d.p2 = ensure_points_have_distance(controls.intermediateItem->d.p1, controls.intermediateItem->d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);
            controls.intermediateItem->lastUpdateTime = std::chrono::steady_clock::now();
            if(!drawP.controls.leftClickHeld) {
                auto editTool = std::make_unique<EditTool>(drawP);
                editTool->edit_start(controls.intermediateItem);
                drawP.switch_to_tool_ptr(std::move(editTool));
                return;
            }
            else {
                controls.intermediateItem->client_send_update(drawP);
                controls.intermediateItem->commit_update(drawP);
            }
            break;
        }
    }
}

bool TextBoxTool::prevent_undo_or_redo() {
    return controls.intermediateItem != nullptr;
}

void TextBoxTool::commit() {
    if(controls.intermediateItem && controls.intermediateItem->collabListInfo.lock()) {
        controls.intermediateItem->client_send_update(drawP);
        controls.intermediateItem->commit_update(drawP);
    }
    controls.intermediateItem = nullptr; 
}

void TextBoxTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}
