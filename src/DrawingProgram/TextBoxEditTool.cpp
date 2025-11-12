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
#include <modules/skparagraph/include/TextStyle.h>
#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skunicode/include/SkUnicode_icu.h>
#include <memory>
#include "EditTool.hpp"

using namespace RichText;

TextBoxEditTool::TextBoxEditTool(DrawingProgram& initDrawP):
    DrawingProgramEditToolBase(initDrawP)
{}

bool TextBoxEditTool::edit_gui(const std::shared_ptr<DrawComponent>& comp) {
    std::shared_ptr<DrawTextBox> a = std::static_pointer_cast<DrawTextBox>(comp);
    Toolbar& t = drawP.world.main.toolbar;

    t.gui.push_id("edit tool text");
    t.gui.text_label_centered("Edit Text");

    t.gui.left_to_right_line_layout([&]() {
        t.gui.text_label("Font");
        if(t.gui.font_picker("font picker", &newFontName)) {
            currentMods[TextStyleModifier::ModifierType::FONT_FAMILIES] = std::make_shared<FontFamiliesTextStyleModifier>(std::vector<SkString>{SkString{newFontName.c_str(), newFontName.size()}});
            add_undo_if_selecting_area(a, [&]() {a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::FONT_FAMILIES]);});
        }
    });
    
    t.gui.left_to_right_line_centered_layout([&]() {
        if(t.gui.svg_icon_button("Bold button", "data/icons/RemixIcon/bold.svg", newIsBold)) {
            newIsBold = !newIsBold;
            currentMods[TextStyleModifier::ModifierType::WEIGHT] = std::make_shared<WeightTextStyleModifier>(newIsBold ? SkFontStyle::Weight::kBold_Weight : SkFontStyle::Weight::kNormal_Weight);
            add_undo_if_selecting_area(a, [&]() {a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::WEIGHT]);});
        }

        if(t.gui.svg_icon_button("Italic button", "data/icons/RemixIcon/italic.svg", newIsItalic)) {
            newIsItalic = !newIsItalic;
            currentMods[TextStyleModifier::ModifierType::SLANT] = std::make_shared<SlantTextStyleModifier>(newIsItalic ? SkFontStyle::Slant::kItalic_Slant : SkFontStyle::Slant::kUpright_Slant);
            add_undo_if_selecting_area(a, [&]() {a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::SLANT]);});
        }

        bool decorationEdited = false;
        if(t.gui.svg_icon_button("Underline button", "data/icons/RemixIcon/underline.svg", newIsUnderlined)) {
            newIsUnderlined = !newIsUnderlined;
            decorationEdited = true;
        }
        if(t.gui.svg_icon_button("Strikethrough button", "data/icons/RemixIcon/strikethrough.svg", newIsLinethrough)) {
            newIsLinethrough = !newIsLinethrough;
            decorationEdited = true;
        }
        if(t.gui.svg_icon_button("Overline button", "data/icons/RemixIcon/overline.svg", newIsOverline)) {
            newIsOverline = !newIsOverline;
            decorationEdited = true;
        }
        if(decorationEdited) {
            currentMods[TextStyleModifier::ModifierType::DECORATION] = std::make_shared<DecorationTextStyleModifier>(get_new_decoration_value());
            add_undo_if_selecting_area(a, [&]() {a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::DECORATION]);});
        }
    });

    t.gui.left_to_right_line_centered_layout([&]() {
        if(t.gui.svg_icon_button("Align left button", "data/icons/RemixIcon/align-left.svg", currentPStyle.textAlignment == skia::textlayout::TextAlign::kLeft))
            add_undo([&](){a->textBox->set_text_alignment_between(a->cursor->selectionBeginPos.fParagraphIndex, a->cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextAlign::kLeft);});
        if(t.gui.svg_icon_button("Align center button", "data/icons/RemixIcon/align-center.svg", currentPStyle.textAlignment == skia::textlayout::TextAlign::kCenter))
            add_undo([&](){a->textBox->set_text_alignment_between(a->cursor->selectionBeginPos.fParagraphIndex, a->cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextAlign::kCenter);});
        if(t.gui.svg_icon_button("Align right button", "data/icons/RemixIcon/align-right.svg", currentPStyle.textAlignment == skia::textlayout::TextAlign::kRight))
            add_undo([&](){a->textBox->set_text_alignment_between(a->cursor->selectionBeginPos.fParagraphIndex, a->cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextAlign::kRight);});
        if(t.gui.svg_icon_button("Align justify button", "data/icons/RemixIcon/align-justify.svg", currentPStyle.textAlignment == skia::textlayout::TextAlign::kJustify))
            add_undo([&](){a->textBox->set_text_alignment_between(a->cursor->selectionBeginPos.fParagraphIndex, a->cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextAlign::kJustify);});
        if(t.gui.svg_icon_button("Text direction left", "data/icons/RemixIcon/text-direction-l.svg", currentPStyle.textDirection == skia::textlayout::TextDirection::kLtr))
            add_undo([&](){a->textBox->set_text_direction_between(a->cursor->selectionBeginPos.fParagraphIndex, a->cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextDirection::kLtr);});
        if(t.gui.svg_icon_button("Text direction right", "data/icons/RemixIcon/text-direction-r.svg", currentPStyle.textDirection == skia::textlayout::TextDirection::kRtl))
            add_undo([&](){a->textBox->set_text_direction_between(a->cursor->selectionBeginPos.fParagraphIndex, a->cursor->selectionEndPos.fParagraphIndex, skia::textlayout::TextDirection::kRtl);});
    });

    if(t.gui.slider_scalar_field<uint32_t>("Font Size Slider", "Font Size", &newFontSize, 3, 100)) {
        hold_undo_data("Font Size", a);
        currentMods[TextStyleModifier::ModifierType::SIZE] = std::make_shared<SizeTextStyleModifier>(newFontSize);
        a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::SIZE]);
    }
    else
        release_undo_data("Font Size");

    t.gui.left_to_right_line_layout([&]() {
        CLAY({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(40), .height = CLAY_SIZING_FIXED(40)}}}) {
            if(t.gui.color_button("Text Color", &newTextColor, &newTextColor == t.colorRight)) 
                t.color_selector_right(&newTextColor == t.colorRight ? nullptr : &newTextColor);
        }
        t.gui.text_label("Text Color");
    });

    if(&newTextColor == t.colorRight)
        hold_undo_data("Text Color", a);
    else
        release_undo_data("Text Color");

    if((&newTextColor == t.colorRight) && t.isUpdatingColorRight) {
        currentMods[TextStyleModifier::ModifierType::COLOR] = std::make_shared<ColorTextStyleModifier>(newTextColor);
        a->textBox->set_text_style_modifier_between(a->cursor->selectionBeginPos, a->cursor->selectionEndPos, currentMods[TextStyleModifier::ModifierType::COLOR]);
    }

    t.gui.pop_id();

    if(a->textBox->inputChangedTextBox)
        set_styles_at_selection(a);

    // NOTE: There should be a system that periodically sends updates even if no changes are made, since unreliable channels can drop update data
    bool oldInputChangedTextBox = a->textBox->inputChangedTextBox;
    a->textBox->inputChangedTextBox = false;
    return oldInputChangedTextBox;
}

void TextBoxEditTool::hold_undo_data(const std::string& undoName, const std::shared_ptr<DrawTextBox>& a) {
    auto it = undoHeldData.find(undoName);
    if(it != undoHeldData.end()) {
        std::pair<RichText::TextBox::Cursor, RichText::TextData>& undoData = it->second;
        if(undoData.first != *a->cursor)
            undoHeldData.erase(it);
    }
    else if(a->cursor->selectionBeginPos != a->cursor->selectionEndPos)
        undoHeldData.emplace(undoName, std::pair<RichText::TextBox::Cursor, RichText::TextData>{*a->cursor, a->textBox->get_rich_text_data()});
}

void TextBoxEditTool::release_undo_data(const std::string& undoName) {
    auto it = undoHeldData.find(undoName);
    if(it != undoHeldData.end()) {
        std::pair<RichText::TextBox::Cursor, RichText::TextData>& undoData = it->second;
        InputManager& input = drawP.world.main.input;
        input.text.add_textbox_undo(undoData.first, undoData.second);
        undoHeldData.erase(it);
    }
}

void TextBoxEditTool::add_undo_if_selecting_area(const std::shared_ptr<DrawTextBox>& a, const std::function<void()>& func) {
    if(a->cursor->selectionBeginPos == a->cursor->selectionEndPos)
        func();
    else {
        InputManager& input = drawP.world.main.input;
        input.text.do_textbox_operation_with_undo(func);
    }
}

void TextBoxEditTool::add_undo(const std::function<void()>& func) {
    InputManager& input = drawP.world.main.input;
    input.text.do_textbox_operation_with_undo(func);
}

uint8_t TextBoxEditTool::get_new_decoration_value() {
    uint8_t toRet = skia::textlayout::TextDecoration::kNoDecoration;
    if(newIsUnderlined)
        toRet |= skia::textlayout::TextDecoration::kUnderline;
    if(newIsLinethrough)
        toRet |= skia::textlayout::TextDecoration::kLineThrough;
    if(newIsOverline)
        toRet |= skia::textlayout::TextDecoration::kOverline;
    return toRet;
}

void TextBoxEditTool::set_styles_at_selection(const std::shared_ptr<DrawTextBox>& a) {
    TextPosition start = std::min(a->cursor->selectionBeginPos, a->cursor->selectionEndPos);
    TextPosition end = std::max(a->cursor->selectionBeginPos, a->cursor->selectionEndPos);
    if(start == end)
        currentMods = a->textBox->get_mods_used_at_pos(a->textBox->move(TextBox::Movement::LEFT, start));
    else
        currentMods = a->textBox->get_mods_used_at_pos(start);

    newFontName = std::static_pointer_cast<FontFamiliesTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::FONT_FAMILIES])->get_families().back().c_str();
    newFontSize = std::static_pointer_cast<SizeTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::SIZE])->get_size();
    newTextColor = std::static_pointer_cast<ColorTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::COLOR])->get_color();
    newIsBold = std::static_pointer_cast<WeightTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::WEIGHT])->get_weight() == SkFontStyle::Weight::kBold_Weight;
    newIsItalic = std::static_pointer_cast<SlantTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::SLANT])->get_slant() == SkFontStyle::Slant::kItalic_Slant;
    newIsUnderlined = std::static_pointer_cast<DecorationTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::DECORATION])->get_decoration_value() & skia::textlayout::TextDecoration::kUnderline;
    newIsLinethrough = std::static_pointer_cast<DecorationTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::DECORATION])->get_decoration_value() & skia::textlayout::TextDecoration::kLineThrough;
    newIsOverline = std::static_pointer_cast<DecorationTextStyleModifier>(currentMods[TextStyleModifier::ModifierType::DECORATION])->get_decoration_value() & skia::textlayout::TextDecoration::kOverline;

    currentPStyle = a->textBox->get_paragraph_style_data_at(start.fParagraphIndex);
}

void TextBoxEditTool::commit_edit_updates(const std::shared_ptr<DrawComponent>& comp, std::any& prevData) {
    std::shared_ptr<DrawTextBox> a = std::static_pointer_cast<DrawTextBox>(comp);

    a->d.editing = false;
    auto pData = std::any_cast<TextBoxEditToolAllData>(prevData);
    TextBoxEditToolAllData cData = get_all_data(a);
    TextData richText = a->textBox->get_rich_text_data();
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
    cur = std::make_shared<TextBox::Cursor>();
    Vector2f textSelectPos = a->get_mouse_pos(drawP);
    textbox->process_mouse_left_button(*cur, textSelectPos, true, false, false);
    prevData = get_all_data(a);
    a->d.editing = true;
    a->textBox->inputChangedTextBox = true;

    a->commit_update(drawP);
    a->client_send_update(drawP, false);

    set_styles_at_selection(a);

    editTool.add_point_handle({&a->d.p1, nullptr, &a->d.p2});
    editTool.add_point_handle({&a->d.p2, &a->d.p1, nullptr});
}

bool TextBoxEditTool::edit_update(const std::shared_ptr<DrawComponent>& comp) {
    std::shared_ptr<DrawTextBox> a = std::static_pointer_cast<DrawTextBox>(comp);

    SCollision::ColliderCollection<float> mousePointCollection;
    mousePointCollection.circle.emplace_back(drawP.world.main.input.mouse.pos, 1.0f);
    mousePointCollection.recalculate_bounds();

    a->init_text_box(drawP);

    InputManager& input = drawP.world.main.input;

    bool collidesWithBox = a->collides_with_cam_coords(drawP.world.drawData.cam.c, mousePointCollection);

    input.text.set_rich_text_box_input(a->textBox, a->cursor, currentMods);
    a->textBox->process_mouse_left_button(*a->cursor, a->get_mouse_pos(drawP), drawP.controls.leftClick && collidesWithBox, drawP.controls.leftClickHeld, input.key(InputManager::KEY_GENERIC_LSHIFT).held);

    return true;
}
