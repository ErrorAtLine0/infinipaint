#include "TextBoxEditTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "Helpers/SCollision.hpp"
#include "../CollabTextBox/CollabTextBox.hpp"
#include <cereal/types/vector.hpp>
#include <memory>
#include "EditTool.hpp"

TextBoxEditTool::TextBoxEditTool(DrawingProgram& initDrawP):
    DrawingProgramEditToolBase(initDrawP)
{}

bool TextBoxEditTool::edit_gui(const std::shared_ptr<DrawComponent>& comp) {
    std::shared_ptr<DrawTextBox> a = std::static_pointer_cast<DrawTextBox>(comp);
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
    bool editHappened = a->d != oldData;
    oldData = a->d;
    return editHappened;
}

void TextBoxEditTool::commit_edit_updates(const std::shared_ptr<DrawComponent>& comp, std::any& prevData) {
    std::shared_ptr<DrawTextBox> a = std::static_pointer_cast<DrawTextBox>(comp);

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
            return true;
        },
        [&, a, cData]() {
            a->d = cData;
            a->set_textbox_string(cData.currentText);
            a->d.editing = false;
            a->client_send_update(drawP);
            a->commit_update(drawP);
            return true;
        }
    });
}

void TextBoxEditTool::edit_start(EditTool& editTool, const std::shared_ptr<DrawComponent>& comp, std::any& prevData) {
    std::shared_ptr<DrawTextBox> a = std::static_pointer_cast<DrawTextBox>(comp);
    auto& cur = a->d.cursor;
    auto& textbox = a->textBox;
    Vector2f textSelectPos = a->get_mouse_pos(drawP);
    SkIPoint p = convert_vec2<SkIPoint>(textSelectPos.cast<int32_t>());
    cur.pos = cur.selectionBeginPos = cur.selectionEndPos = textbox.getPosition(p);
    prevData = a->d;
    a->d.editing = true;
    a->client_send_update(drawP);
    editTool.add_point_handle({&a->d.p1, nullptr, &a->d.p2});
    editTool.add_point_handle({&a->d.p2, &a->d.p1, nullptr});
}

bool TextBoxEditTool::edit_update(const std::shared_ptr<DrawComponent>& comp) {
    std::shared_ptr<DrawTextBox> a = std::static_pointer_cast<DrawTextBox>(comp);

    SCollision::ColliderCollection<float> mousePointCollection;
    mousePointCollection.circle.emplace_back(drawP.world.main.input.mouse.pos, 1.0f);
    mousePointCollection.recalculate_bounds();

    a->textBox.setWidth(std::abs(a->d.p2.x() - a->d.p1.x()));

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

// Passing a function in, just in case we have to do something before editing the text
void TextBoxEditTool::edit_text(std::function<void()> toRun, const std::shared_ptr<DrawTextBox>& textBox) {
    toRun();
    textBox->update_contained_string(drawP);
    update_textbox_network(textBox);
}

void TextBoxEditTool::update_textbox_network(const std::shared_ptr<DrawTextBox>& textBox) {
    textBox->client_send_update(drawP);
    textBox->commit_update(drawP);
}
