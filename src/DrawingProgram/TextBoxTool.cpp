#include "TextBoxTool.hpp"
#include <chrono>
#include "DrawingProgram.hpp"
#include "../Server/CommandList.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "Helpers/Serializers.hpp"
#include "../CollabTextBox/CollabTextBox.hpp"
#include "../SharedTypes.hpp"
#include <cereal/types/vector.hpp>
#include <memory>
#include <ranges>

TextBoxTool::TextBoxTool(DrawingProgram& initDrawP):
    drawP(initDrawP)
{
}

void TextBoxTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("textbox tool");
    t.gui.text_label_centered("Textbox");
    t.gui.slider_scalar_field("Text Relative Size", "Text Size", &controls.textRelativeSize, 5.0f, 100.0f);
    t.gui.pop_id();
}

void TextBoxTool::reset_tool() {
    commit();
    controls.drawStage = 0;
}

bool TextBoxTool::edit_gui(const std::shared_ptr<DrawTextBox>& a) {
    DrawTextBox::Data oldData = a->d;
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("edit tool text");
    t.gui.text_label_centered("Edit Text");
    t.gui.slider_scalar_field("Text Relative Size", "Text Size", &a->d.textSize, 5.0f, 100.0f);
    t.gui.left_to_right_line_layout([&]() {
        CLAY({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(40), .height = CLAY_SIZING_FIXED(40)}}}) {
            if(t.gui.color_button("Text Color", &a->d.textColor, &a->d.textColor == t.colorRight))
                t.color_selector_right(&a->d.textColor == t.colorRight ? nullptr : &a->d.textColor);
        }
        t.gui.text_label("Text Color");
    });
    t.gui.pop_id();
    return a->d != oldData;
}

void TextBoxTool::commit_edit_updates(const std::shared_ptr<DrawTextBox>& a, std::any& prevData) {
    a->d.editing = false;
    DrawTextBox::Data pData = std::any_cast<DrawTextBox::Data>(prevData);
    DrawTextBox::Data cData = a->d; 
    drawP.world.undo.push(UndoManager::UndoRedoPair{
        [&, a, pData]() {
            a->d = pData;
            a->set_textbox_string(pData.currentText);
            a->d.editing = false;
            a->client_send_update(drawP);
            a->commit_update(drawP);
            drawP.reset_tools();
            return true;
        },
        [&, a, cData]() {
            a->d = cData;
            a->set_textbox_string(cData.currentText);
            a->d.editing = false;
            a->client_send_update(drawP);
            a->commit_update(drawP);
            drawP.reset_tools();
            return true;
        }
    });
}

void TextBoxTool::edit_start(const std::shared_ptr<DrawTextBox>& a, std::any& prevData) {
    auto& cur = a->d.cursor;
    auto& textbox = a->textBox;
    Vector2f textSelectPos = a->get_mouse_pos(drawP);
    SkIPoint p = convert_vec2<SkIPoint>(textSelectPos.cast<int32_t>());
    cur.pos = cur.selectionBeginPos = cur.selectionEndPos = textbox.getPosition(p);
    prevData = a->d;
    a->d.editing = true;
    a->client_send_update(drawP);
    drawP.editTool.add_point_handle({&a->d.p1, nullptr, &a->d.p2});
    drawP.editTool.add_point_handle({&a->d.p2, &a->d.p1, nullptr});
}

bool TextBoxTool::edit_update(const std::shared_ptr<DrawTextBox>& a) {
    SCollision::ColliderCollection<float> mousePointCollection;
    mousePointCollection.circle.emplace_back(drawP.world.main.input.mouse.pos, 1.0f);
    mousePointCollection.recalculate_bounds();

    a->textBox.setWidth(std::abs(a->d.p2.x() - a->d.p1.x()));
    a->create_collider();

    InputManager& input = drawP.world.main.input;
    auto& textbox = a->textBox;
    auto& cur = a->d.cursor;

    bool moved = false;
    bool movedHeld = false;

    bool collidesWithBox = false;
    if(a->collides_with_cam_coords(drawP.world.drawData.cam.c, mousePointCollection)) {
        //drawP.world.main.input.cursorIcon = InputManager::SystemCursorType::TEXT;
        collidesWithBox = true;
    }

    if(drawP.controls.leftClick) {
        if(collidesWithBox) {
            Vector2f textSelectPos = a->get_mouse_pos(drawP);
            SkIPoint p = convert_vec2<SkIPoint>(textSelectPos.cast<int32_t>());
            cur.pos = cur.selectionBeginPos = textbox.getPosition(p);
            moved = true;
        }
    }
    else if(drawP.controls.leftClickHeld) {
        if(collidesWithBox) {
            Vector2f textSelectPos = a->get_mouse_pos(drawP);
            SkIPoint p = convert_vec2<SkIPoint>(textSelectPos.cast<int32_t>());
            cur.pos = cur.selectionBeginPos = textbox.getPosition(p);
        }
        movedHeld = true;
    }


    if(input.key(InputManager::KEY_DRAW_UNSELECT).repeat)
        reset_tool();

    if(input.key(InputManager::KEY_TEXT_LEFT).repeat)
        cur.pos = cur.selectionBeginPos = textbox.move(CollabTextBox::Movement::kLeft, cur.selectionBeginPos);
    if(input.key(InputManager::KEY_TEXT_RIGHT).repeat)
        cur.pos = cur.selectionBeginPos = textbox.move(CollabTextBox::Movement::kRight, cur.selectionBeginPos);
    if(input.key(InputManager::KEY_TEXT_UP).repeat)
        cur.pos = cur.selectionBeginPos = textbox.move(CollabTextBox::Movement::kUp, cur.selectionBeginPos);
    if(input.key(InputManager::KEY_TEXT_DOWN).repeat)
        cur.pos = cur.selectionBeginPos = textbox.move(CollabTextBox::Movement::kDown, cur.selectionBeginPos);
    if(input.key(InputManager::KEY_TEXT_HOME).repeat)
        cur.pos = cur.selectionBeginPos = textbox.move(CollabTextBox::Movement::kHome, cur.selectionBeginPos);

    moved = moved || input.key(InputManager::KEY_TEXT_DOWN).repeat || input.key(InputManager::KEY_TEXT_UP).repeat || input.key(InputManager::KEY_TEXT_RIGHT).repeat || input.key(InputManager::KEY_TEXT_LEFT).repeat || input.key(InputManager::KEY_TEXT_HOME).repeat;
    if(moved && !input.key(InputManager::KEY_TEXT_SHIFT).held && !movedHeld)
        cur.selectionEndPos = cur.selectionBeginPos;

    if(moved || movedHeld)
        update_textbox_network(a);

    if(input.key(InputManager::KEY_TEXT_BACKSPACE).repeat) {
        edit_text([&]() {
            if(cur.selectionBeginPos != cur.selectionEndPos)
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.selectionBeginPos, cur.selectionEndPos);
            else
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.pos, textbox.move(CollabTextBox::Movement::kLeft, cur.pos));
        }, a);
    }
    if(input.key(InputManager::KEY_TEXT_DELETE).repeat) {
        edit_text([&]() {
            if(cur.selectionBeginPos != cur.selectionEndPos)
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.selectionBeginPos, cur.selectionEndPos);
            else
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.pos, textbox.move(CollabTextBox::Movement::kRight, cur.pos));
        }, a);
    }
    if(input.get_clipboard_paste_happened()) {
        edit_text([&]() {
            if(cur.selectionBeginPos != cur.selectionEndPos)
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.selectionBeginPos, cur.selectionEndPos);
            std::string clipboard = input.get_clipboard_str();
            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.insert(cur.pos, clipboard);
        }, a);
    }
    if(input.key(InputManager::KEY_TEXT_ENTER).repeat) {
        edit_text([&]() {
            if(cur.selectionBeginPos != cur.selectionEndPos)
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.selectionBeginPos, cur.selectionEndPos);
            cur.pos = textbox.insert(cur.pos, "\n");
            cur.pos.fParagraphIndex++;
            cur.pos.fTextByteIndex = 0;
            cur.selectionBeginPos = cur.selectionEndPos = cur.pos;
        }, a);
    }
    if(input.key(InputManager::KEY_TEXT_COPY).repeat) {
        std::string clipboardData = textbox.copy(cur.selectionBeginPos, cur.selectionEndPos);
        input.set_clipboard_str(clipboardData);
    }
    if(input.key(InputManager::KEY_TEXT_CUT).repeat) {
        edit_text([&]() {
            std::string clipboardData = textbox.copy(cur.selectionBeginPos, cur.selectionEndPos);
            input.set_clipboard_str(clipboardData);
            if(cur.selectionBeginPos != cur.selectionEndPos)
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.selectionBeginPos, cur.selectionEndPos);
        }, a);
    }
    if(input.key(InputManager::KEY_TEXT_SELECTALL).repeat) {
        cur.selectionEndPos.fParagraphIndex = textbox.lineCount() == 0 ? 0 : textbox.lineCount() - 1;
        cur.selectionEndPos.fTextByteIndex = textbox.line(cur.selectionEndPos.fParagraphIndex).size();
        cur.pos = cur.selectionEndPos;
        cur.selectionBeginPos.fTextByteIndex = 0;
        cur.selectionBeginPos.fParagraphIndex = 0;
        update_textbox_network(a);
    }
    if(!input.text.newInput.empty()) {
        edit_text([&]() {
            if(cur.selectionBeginPos != cur.selectionEndPos)
                cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.remove(cur.selectionBeginPos, cur.selectionEndPos);
            cur.selectionEndPos = cur.selectionBeginPos = cur.pos = textbox.insert(cur.pos, input.text.newInput);
        }, a);
    }

    input.text.set_accepting_input();
    return true;
}

void TextBoxTool::tool_update() {
    switch(controls.drawStage) {
        case 0: {
            if(drawP.controls.leftClick) {
                controls.intermediateItem = std::make_shared<DrawTextBox>();
                controls.startAt = drawP.world.main.input.mouse.pos;
                controls.endAt = controls.startAt;
                controls.intermediateItem->d.p1 = controls.intermediateItem->d.p2 = controls.startAt;
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
            if(drawP.world.main.input.key(InputManager::KEY_EQUAL_DIMENSIONS).held) {
                float height = std::fabs(controls.startAt.y() - newPos.y());
                newPos.x() = controls.startAt.x() + (((newPos.x() - controls.startAt.x()) < 0.0f ? -1.0f : 1.0f) * height);
            }
            controls.endAt = newPos;
            controls.intermediateItem->d.p1 = cwise_vec_min(controls.endAt, controls.startAt);
            controls.intermediateItem->d.p2 = cwise_vec_max(controls.endAt, controls.startAt);
            controls.intermediateItem->lastUpdateTime = std::chrono::steady_clock::now();
            if(!drawP.controls.leftClickHeld) {
                auto i = controls.intermediateItem;
                commit();
                drawP.reset_tools();
                drawP.controls.selectedTool = DrawingProgram::TOOL_EDIT;
                drawP.controls.previousSelected = DrawingProgram::TOOL_EDIT;
                drawP.editTool.edit_start(i);
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

// Passing a function in, just in case we have to do something before editing the text
void TextBoxTool::edit_text(std::function<void()> toRun, const std::shared_ptr<DrawTextBox>& textBox) {
    toRun();
    textBox->update_contained_string(drawP);
    update_textbox_network(textBox);
}

void TextBoxTool::update_textbox_network(const std::shared_ptr<DrawTextBox>& textBox) {
    textBox->client_send_update(drawP);
    textBox->commit_update(drawP);
}

void TextBoxTool::commit() {
    if(controls.intermediateItem && controls.intermediateItem->collabListInfo.lock()) {
        controls.intermediateItem->client_send_update(drawP);
        controls.intermediateItem->commit_update(drawP);
    }
    controls.intermediateItem = nullptr; 
}

void TextBoxTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(controls.intermediateItem)
        controls.intermediateItem->draw(canvas, drawData);
}
