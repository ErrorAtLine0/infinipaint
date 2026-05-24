/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "TextBoxCanvasComponent.hpp"
#include "Helpers/ConvertVec.hpp"
#include <cereal/types/string.hpp>
#include "../MainProgram.hpp"
#include "Helpers/SCollision.hpp"
#include "../DrawingProgram/DrawingProgram.hpp"
#include "CanvasComponentContainer.hpp"
#include "../DrawCollision.hpp"
#include <include/pathops/SkPathOps.h>

using namespace RichText;

TextBoxCanvasComponent::TextBoxCanvasComponent():
    textBox(std::make_shared<TextBox>()),
    cursor(std::make_shared<TextBox::Cursor>())
{
    skia::textlayout::TextStyle tStyle;
    textBox->set_initial_text_style(tStyle);

    textBox->set_initial_text_style_modifier(std::make_shared<ColorTextStyleModifier>(Vector4f{1.0f, 1.0f, 1.0f, 1.0f}));
    textBox->set_initial_text_style_modifier(std::make_shared<SizeTextStyleModifier>(18.0f));
    textBox->set_initial_text_style_modifier(std::make_shared<FontFamiliesTextStyleModifier>(std::vector<SkString>{SkString{"Roboto"}}));
}

CanvasComponentType TextBoxCanvasComponent::get_type() const {
    return CanvasComponentType::TEXTBOX;
}

void TextBoxCanvasComponent::save(cereal::PortableBinaryOutputArchive& a) const {
    a(d.editing, d.p1, d.p2, textBox->get_rich_text_data(), *cursor);
}

void TextBoxCanvasComponent::load(cereal::PortableBinaryInputArchive& a) {
    TextData richText;
    a(d.editing, d.p1, d.p2, richText, *cursor);
    textBox->set_rich_text_data(richText);
}

void TextBoxCanvasComponent::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(d.p1, d.p2, textBox->get_rich_text_data());
}

void TextBoxCanvasComponent::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    if(version >= VersionNumber(0, 3, 0)) {
        TextData richText;
        a(d.p1, d.p2, richText);
        textBox->set_rich_text_data(richText);
    }
    else {
        bool loadedEditing; // Unused
        Vector4f textColor;
        float textSize;
        RichText::TextBox::Cursor loadedCursor; // Unused, Equivalent to old cursor structure, just paragraph index and text index flipped
        std::string currentText;
        a(loadedEditing, d.p1, d.p2, textColor, textSize, loadedCursor, currentText);
        textBox->insert({0, 0}, currentText);
        textBox->set_text_style_modifier_between({0, 0}, textBox->move(RichText::TextBox::Movement::END, {0, 0}), std::make_shared<ColorTextStyleModifier>(textColor));
        textBox->set_text_style_modifier_between({0, 0}, textBox->move(RichText::TextBox::Movement::END, {0, 0}), std::make_shared<SizeTextStyleModifier>(textSize));
    }
}

std::unique_ptr<CanvasComponent> TextBoxCanvasComponent::get_data_copy() const {
    auto toRet = std::make_unique<TextBoxCanvasComponent>();
    toRet->d = d;
    toRet->d.editing = false;
    *toRet->cursor = *cursor;
    toRet->textBox->set_rich_text_data(textBox->get_rich_text_data());
    return toRet;
}

void TextBoxCanvasComponent::set_data_from(const CanvasComponent& other) {
    auto& otherTextBox = static_cast<const TextBoxCanvasComponent&>(other);
    d = otherTextBox.d;
    *cursor = *otherTextBox.cursor;
    textBox->set_rich_text_data(otherTextBox.textBox->get_rich_text_data());
}

void TextBoxCanvasComponent::init_text_box(DrawingProgram& drawP) {
    textBox->set_font_data(drawP.world.main.fonts); // Getting a segfault relating to the paragraph cache means that the font collection hasn't been set yet
    textBox->set_max_width(d.p2.x() - d.p1.x() - TEXTBOX_PADDING * 2.0f);
}

void TextBoxCanvasComponent::draw(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<void>& predrawData) const {
    SkRect clipR = SkRect::MakeLTRB(d.p1.x(), d.p1.y(), d.p2.x(), d.p2.y());
    if(d.editing) {
        SkPaint p;
        p.setAntiAlias(drawData.skiaAA);
        p.setStyle(SkPaint::kStroke_Style);
        p.setStrokeWidth(0.0f);
        p.setColor4f({0.5f, 0.5f, 0.5f, 1.0f});
        canvas->drawRect(clipR, p);
    }
    canvas->clipRect(clipR, drawData.skiaAA);
    canvas->translate(d.p1.x() + TEXTBOX_PADDING, d.p1.y() + TEXTBOX_PADDING);

    TextBox::PaintOpts paintOpts;
    paintOpts.cursorColor = {0.7f, 0.7f, 1.0f};
    if(d.editing && cursor)
        paintOpts.cursor = *cursor;
    paintOpts.skiaAA = drawData.skiaAA;

    textBox->paint(canvas, paintOpts);
}

Vector2f TextBoxCanvasComponent::get_mouse_pos(DrawingProgram& drawP) const {
    return compContainer->coords.get_mouse_pos(drawP.world) - d.p1 - Vector2f{TEXTBOX_PADDING, TEXTBOX_PADDING};
}

void TextBoxCanvasComponent::initialize_draw_data(DrawingProgram& drawP) {
    init_text_box(drawP);
}

bool TextBoxCanvasComponent::collides_within_coords_point(const Vector2f& checkAgainst) const {
    SkPath p = SkPath::Rect(SCollision::AABB<float>(d.p1, d.p2).get_sk_rect());
    bool intersectsAABB = p.getBounds().contains(checkAgainst.x(), checkAgainst.y());
    if(!intersectsAABB)
        return false;
    return p.contains(checkAgainst.x(), checkAgainst.y());
}

bool TextBoxCanvasComponent::collides_within_coords_skpath(const SkPath& checkAgainst) const {
    SkPath p = SkPath::Rect(SCollision::AABB<float>(d.p1, d.p2).get_sk_rect());
    bool intersectsAABB = p.getBounds().intersects(checkAgainst.getBounds());
    if(!intersectsAABB)
        return false;
    std::optional<SkPath> pathIntersectCheck = Op(checkAgainst, p, SkPathOp::kIntersect_SkPathOp);
    return pathIntersectCheck.has_value() && !pathIntersectCheck.value().isEmpty();
}

SCollision::AABB<float> TextBoxCanvasComponent::get_obj_coord_bounds() const {
    return {d.p1, d.p2};
}
