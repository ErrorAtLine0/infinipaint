#include "TextBoxEditTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "Helpers/ConvertVec.hpp"
#include "Helpers/SCollision.hpp"
#include <cereal/types/vector.hpp>
#include <include/core/SkFontStyle.h>
#include <modules/skparagraph/include/DartTypes.h>
#include <modules/skparagraph/include/ParagraphStyle.h>
#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skunicode/include/SkUnicode_icu.h>
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

    newFontName = std::static_pointer_cast<FontFamiliesTextStyleModifier>(modsAtStartOfSelection[TextStyleModifier::ModifierType::FONT_FAMILIES])->families.back().c_str();

    t.gui.left_to_right_line_layout([&]() {
        t.gui.text_label("Font");
        if(t.gui.font_picker("font picker", &newFontName)) {
            auto fontFamilyMod = std::make_shared<FontFamiliesTextStyleModifier>();
            fontFamilyMod->families.clear();
            fontFamilyMod->families.emplace_back(newFontName);
            a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, fontFamilyMod);
        }
    });
    
    t.gui.left_to_right_line_centered_layout([&]() {
        bool isBold = std::static_pointer_cast<WeightTextStyleModifier>(modsAtStartOfSelection[TextStyleModifier::ModifierType::WEIGHT])->get_weight() == SkFontStyle::Weight::kBold_Weight;
        if(t.gui.svg_icon_button("Bold button", "data/icons/RemixIcon/bold.svg", isBold)) {
            auto boldMod = std::make_shared<WeightTextStyleModifier>();
            boldMod->set_weight(isBold ? SkFontStyle::Weight::kNormal_Weight : SkFontStyle::Weight::kBold_Weight);
            a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, boldMod);
        }

        bool isItalic = std::static_pointer_cast<SlantTextStyleModifier>(modsAtStartOfSelection[TextStyleModifier::ModifierType::SLANT])->get_slant() == SkFontStyle::Slant::kItalic_Slant;
        if(t.gui.svg_icon_button("Italic button", "data/icons/RemixIcon/italic.svg", isItalic)) {
            auto italicMod = std::make_shared<SlantTextStyleModifier>();
            italicMod->set_slant(isItalic ? SkFontStyle::Slant::kUpright_Slant : SkFontStyle::Slant::kItalic_Slant);
            a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, italicMod);
        }

        uint8_t currentDecorationValue = std::static_pointer_cast<DecorationTextStyleModifier>(modsAtStartOfSelection[TextStyleModifier::ModifierType::DECORATION])->decorationValue;
        bool isUnderlined = currentDecorationValue & skia::textlayout::TextDecoration::kUnderline;
        if(t.gui.svg_icon_button("Underline button", "data/icons/RemixIcon/underline.svg", isUnderlined)) {
            auto decorationMod = std::make_shared<DecorationTextStyleModifier>();
            decorationMod->decorationValue = isUnderlined ? (currentDecorationValue & ~static_cast<uint8_t>(skia::textlayout::TextDecoration::kUnderline)) : currentDecorationValue | static_cast<uint8_t>(skia::textlayout::TextDecoration::kUnderline);
            a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, decorationMod);
        }

        bool isLinethrough = currentDecorationValue & skia::textlayout::TextDecoration::kLineThrough;
        if(t.gui.svg_icon_button("Strikethrough button", "data/icons/RemixIcon/strikethrough.svg", isLinethrough)) {
            auto decorationMod = std::make_shared<DecorationTextStyleModifier>();
            decorationMod->decorationValue = isLinethrough ? (currentDecorationValue & ~static_cast<uint8_t>(skia::textlayout::TextDecoration::kLineThrough)) : currentDecorationValue | static_cast<uint8_t>(skia::textlayout::TextDecoration::kLineThrough);
            a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, decorationMod);
        }

        bool isOverline = currentDecorationValue & skia::textlayout::TextDecoration::kOverline;
        if(t.gui.svg_icon_button("Overline button", "data/icons/RemixIcon/overline.svg", isOverline)) {
            auto decorationMod = std::make_shared<DecorationTextStyleModifier>();
            decorationMod->decorationValue = isOverline ? (currentDecorationValue & ~static_cast<uint8_t>(skia::textlayout::TextDecoration::kOverline)) : currentDecorationValue | static_cast<uint8_t>(skia::textlayout::TextDecoration::kOverline);
            a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, decorationMod);
        }
    });

    newFontSize = std::static_pointer_cast<SizeTextStyleModifier>(modsAtStartOfSelection[TextStyleModifier::ModifierType::SIZE])->size;
    if(t.gui.slider_scalar_field<uint32_t>("Font Size Slider", "Font Size", &newFontSize, 3, 100)) {
        auto sizeMod = std::make_shared<SizeTextStyleModifier>();
        sizeMod->size = newFontSize;
        a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, sizeMod);
    }
    t.gui.left_to_right_line_layout([&]() {
        CLAY({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(40), .height = CLAY_SIZING_FIXED(40)}}}) {
            if(t.gui.color_button("Text Color", &newTextColor, &newTextColor == t.colorRight)) {
                t.color_selector_right(&newTextColor == t.colorRight ? nullptr : &newTextColor);
            }
        }
        t.gui.text_label("Text Color");
    });
    if((&newTextColor == t.colorRight) && t.isUpdatingColorRight) {
        auto colorMod = std::make_shared<ColorTextStyleModifier>();
        colorMod->color = newTextColor;
        a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, colorMod);
    }
    newTextColor = std::static_pointer_cast<ColorTextStyleModifier>(modsAtStartOfSelection[TextStyleModifier::ModifierType::COLOR])->color;


    t.gui.pop_id();

    if(a->textBox->inputChangedTextBox)
        set_mods_at_selection(a);

    // NOTE: There should be a system that periodically sends updates even if no changes are made, since unreliable channels can drop update data
    bool oldInputChangedTextBox = a->textBox->inputChangedTextBox;
    a->textBox->inputChangedTextBox = false;
    return oldInputChangedTextBox;
}

void TextBoxEditTool::set_mods_at_selection(const std::shared_ptr<DrawTextBox>& a) {
    RichTextBox::TextPosition start = std::min(a->cursor->selectionBeginPos, a->cursor->selectionEndPos);
    RichTextBox::TextPosition end = std::max(a->cursor->selectionBeginPos, a->cursor->selectionEndPos);
    if(start == end)
        modsAtStartOfSelection = a->textBox->get_mods_used_at_pos(a->textBox->move(RichTextBox::Movement::LEFT, start));
    else
        modsAtStartOfSelection = a->textBox->get_mods_used_at_pos(start);
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
    a->textBox->inputChangedTextBox = true;

    a->commit_update(drawP);
    a->client_send_update(drawP, false);

    set_mods_at_selection(a);

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
