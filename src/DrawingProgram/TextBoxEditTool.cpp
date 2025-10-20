#include "TextBoxEditTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "Helpers/SCollision.hpp"
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
    bool editHappened = (!oldData.has_value()) || (a->d != oldData);
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
            a->client_send_update(drawP, true);
            a->commit_update(drawP);
            return true;
        },
        [&, a, cData]() {
            a->d = cData;
            a->set_textbox_string(cData.currentText);
            a->d.editing = false;
            a->client_send_update(drawP, true);
            a->commit_update(drawP);
            return true;
        }
    });
}

void TextBoxEditTool::edit_start(EditTool& editTool, const std::shared_ptr<DrawComponent>& comp, std::any& prevData) {
    std::shared_ptr<DrawTextBox> a = std::static_pointer_cast<DrawTextBox>(comp);
    auto& cur = a->cursor;
    auto& textbox = a->textBox;
    cur = std::make_shared<RichTextBox::Cursor>();
    Vector2f textSelectPos = a->get_mouse_pos(drawP);
    textbox->process_mouse_left_button(*cur, textSelectPos, true, false, false);
    prevData = a->d;
    a->d.editing = true;
    a->client_send_update(drawP, false);
    editTool.add_point_handle({&a->d.p1, nullptr, &a->d.p2});
    editTool.add_point_handle({&a->d.p2, &a->d.p1, nullptr});
}

bool TextBoxEditTool::edit_update(const std::shared_ptr<DrawComponent>& comp) {
    std::shared_ptr<DrawTextBox> a = std::static_pointer_cast<DrawTextBox>(comp);

    SCollision::ColliderCollection<float> mousePointCollection;
    mousePointCollection.circle.emplace_back(drawP.world.main.input.mouse.pos, 1.0f);
    mousePointCollection.recalculate_bounds();

    a->textBox->set_width(std::abs(a->d.p2.x() - a->d.p1.x()));

    InputManager& input = drawP.world.main.input;

    bool collidesWithBox = a->collides_with_cam_coords(drawP.world.drawData.cam.c, mousePointCollection);

    input.text.set_rich_text_box_input(a->textBox, a->cursor);
    a->textBox->process_mouse_left_button(*a->cursor, a->get_mouse_pos(drawP), drawP.controls.leftClick && collidesWithBox, drawP.controls.leftClickHeld, input.key(InputManager::KEY_GENERIC_LSHIFT).held);

    return true;
}

// Passing a function in, just in case we have to do something before editing the text
void TextBoxEditTool::edit_text(std::function<void()> toRun, const std::shared_ptr<DrawTextBox>& textBox) {
    toRun();
    textBox->update_contained_string(drawP);
    update_textbox_network(textBox);
}

void TextBoxEditTool::update_textbox_network(const std::shared_ptr<DrawTextBox>& textBox) {
    textBox->client_send_update(drawP, false);
    textBox->commit_update(drawP);
}
