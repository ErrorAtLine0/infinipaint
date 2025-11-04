#include "TextBoxEditTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "Helpers/SCollision.hpp"
#include <cereal/types/vector.hpp>
#include <include/core/SkFontStyle.h>
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
    if(t.gui.text_button_wide("Bold button", "Bold")) {
        auto boldMod = std::make_shared<WeightTextStyleModifier>();
        boldMod->set_weight(SkFontStyle::kBold_Weight);
        a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, boldMod);
    }
    if(t.gui.text_button_wide("Italic button", "Italic")) {
        auto italicMod = std::make_shared<SlantTextStyleModifier>();
        italicMod->set_slant(SkFontStyle::Slant::kItalic_Slant);
        a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, italicMod);
    }
    if(t.gui.text_button_wide("Red button", "Red")) {
        auto redMod = std::make_shared<ColorTextStyleModifier>();
        redMod->color = {1.0f, 0.0f, 0.0f, 1.0f};
        a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, redMod);
    }
    if(t.gui.text_button_wide("Green button", "Green")) {
        auto redMod = std::make_shared<ColorTextStyleModifier>();
        redMod->color = {0.0f, 1.0f, 0.0f, 1.0f};
        a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, redMod);
    }
    if(t.gui.text_button_wide("Blue button", "Blue")) {
        auto redMod = std::make_shared<ColorTextStyleModifier>();
        redMod->color = {0.0f, 0.0f, 1.0f, 1.0f};
        a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, redMod);
    }
    //t.gui.left_to_right_line_layout([&]() {
    //    CLAY({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(40), .height = CLAY_SIZING_FIXED(40)}}}) {
    //        if(t.gui.color_button("Text Color", &a->d.textColor, &a->d.textColor == t.colorRight))
    //            t.color_selector_right(&a->d.textColor == t.colorRight ? nullptr : &a->d.textColor);
    //    }
    //    t.gui.text_label("Text Color");
    //});
    t.gui.pop_id();

    return true; // NOTE: In general, the update data should always be sent even if no changes were made, because we're sending the data over an unreliable channel.
}

void TextBoxEditTool::commit_edit_updates(const std::shared_ptr<DrawComponent>& comp, std::any& prevData) {
    std::shared_ptr<DrawTextBox> a = std::static_pointer_cast<DrawTextBox>(comp);

    a->d.editing = false;
    auto pData = std::any_cast<TextBoxEditToolAllData>(prevData);
    TextBoxEditToolAllData cData = get_all_data(a);
    RichTextBox::RichTextData richText = a->textBox->get_rich_text_data();
    drawP.world.undo.push(UndoManager::UndoRedoPair{
        [&drawP = drawP, a, pData]() {
            a->d = pData.textboxData;
            a->textBox->set_rich_text_data(pData.richText);
            a->d.editing = false;
            a->client_send_update(drawP, true);
            a->commit_update(drawP);
            return true;
        },
        [&drawP = drawP, a, cData]() {
            a->d = cData.textboxData;
            a->textBox->set_rich_text_data(cData.richText);
            a->d.editing = false;
            a->client_send_update(drawP, true);
            a->commit_update(drawP);
            return true;
        }
    });
}

TextBoxEditTool::TextBoxEditToolAllData TextBoxEditTool::get_all_data(const std::shared_ptr<DrawTextBox>& a) {
    return {
        .textboxData = a->d,
        .richText = a->textBox->get_rich_text_data()
    };
}

void TextBoxEditTool::edit_start(EditTool& editTool, const std::shared_ptr<DrawComponent>& comp, std::any& prevData) {
    std::shared_ptr<DrawTextBox> a = std::static_pointer_cast<DrawTextBox>(comp);
    auto& cur = a->cursor;
    auto& textbox = a->textBox;
    cur = std::make_shared<RichTextBox::Cursor>();
    Vector2f textSelectPos = a->get_mouse_pos(drawP);
    textbox->process_mouse_left_button(*cur, textSelectPos, true, false, false);
    prevData = get_all_data(a);
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
