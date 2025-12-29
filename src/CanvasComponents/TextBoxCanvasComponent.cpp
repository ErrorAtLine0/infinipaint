#include "TextBoxCanvasComponent.hpp"
#include "Helpers/ConvertVec.hpp"
#include <cereal/types/string.hpp>
#include "../MainProgram.hpp"
#include "Helpers/SCollision.hpp"
#include "../DrawingProgram/DrawingProgram.hpp"
#include "CanvasComponentContainer.hpp"
#include "../DrawCollision.hpp"

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
    if(version < VersionNumber(0, 3, 0)) {
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
    else {
        TextData richText;
        a(d.p1, d.p2, richText);
        textBox->set_rich_text_data(richText);
    }
}

std::unique_ptr<CanvasComponent> TextBoxCanvasComponent::get_data_copy() const {
    auto toRet = std::make_unique<TextBoxCanvasComponent>();
    toRet->d = d;
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
    textBox->set_width(d.p2.x() - d.p1.x() - TEXTBOX_PADDING * 2.0f);
}

void TextBoxCanvasComponent::draw(SkCanvas* canvas, const DrawData& drawData) const {
    SkRect clipR = SkRect::MakeLTRB(d.p1.x(), d.p1.y(), d.p2.x(), d.p2.y());
    if(d.editing) {
        SkPaint p;
        p.setStyle(SkPaint::kStroke_Style);
        p.setStrokeWidth(0.0f);
        p.setColor4f({0.5f, 0.5f, 0.5f, 1.0f});
        canvas->drawRect(clipR, p);
    }
    canvas->clipRect(clipR);
    canvas->translate(d.p1.x() + TEXTBOX_PADDING, d.p1.y() + TEXTBOX_PADDING);

    TextBox::PaintOpts paintOpts;
    paintOpts.cursorColor = {0.7f, 0.7f, 1.0f};
    if(d.editing && cursor)
        paintOpts.cursor = *cursor;

    textBox->paint(canvas, paintOpts);
}

Vector2f TextBoxCanvasComponent::get_mouse_pos(DrawingProgram& drawP) const {
    return compContainer->coords.get_mouse_pos(drawP.world) - d.p1 - Vector2f{TEXTBOX_PADDING, TEXTBOX_PADDING};
}

void TextBoxCanvasComponent::initialize_draw_data(DrawingProgram& drawP) {
    init_text_box(drawP);
    create_collider();
}

bool TextBoxCanvasComponent::collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const {
    return collisionTree.is_collide(checkAgainst);
}

void TextBoxCanvasComponent::create_collider() {
    using namespace SCollision;
    ColliderCollection<float> strokeObjects;
    std::array<Vector2f, 4> newT = triangle_from_rect_points(d.p1, d.p2);
    strokeObjects.triangle.emplace_back(newT[0], newT[1], newT[2]);
    strokeObjects.triangle.emplace_back(newT[2], newT[3], newT[0]);
    collisionTree.clear();
    collisionTree.calculate_bvh_recursive(strokeObjects);
}

SCollision::AABB<float> TextBoxCanvasComponent::get_obj_coord_bounds() const {
    return collisionTree.objects.bounds;
}
