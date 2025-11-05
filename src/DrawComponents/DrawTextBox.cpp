#include "DrawTextBox.hpp"
#include "Helpers/ConvertVec.hpp"
#include <cereal/types/string.hpp>

#ifndef IS_SERVER
#include "../MainProgram.hpp"
#include "Helpers/SCollision.hpp"
#include "../DrawingProgram/DrawingProgram.hpp"
#include "../DrawCollision.hpp"
#endif

DrawTextBox::DrawTextBox():
    textBox(std::make_shared<RichTextBox>()),
    cursor(std::make_shared<RichTextBox::Cursor>())
{
    skia::textlayout::TextStyle tStyle;
    textBox->set_initial_text_style(tStyle);

    auto colorMod = std::make_shared<ColorTextStyleModifier>();
    colorMod->color = {1.0f, 1.0f, 1.0f, 1.0f};
    textBox->set_initial_text_style_modifier(colorMod);

    auto textSizeMod = std::make_shared<SizeTextStyleModifier>();
    textSizeMod->size = 16.0f;
    textBox->set_initial_text_style_modifier(textSizeMod);

    auto fontFamilyMod = std::make_shared<FontFamiliesTextStyleModifier>();
    fontFamilyMod->families = {SkString{"Roboto"}};
    textBox->set_initial_text_style_modifier(fontFamilyMod);
}

DrawComponentType DrawTextBox::get_type() const {
    return DRAWCOMPONENT_TEXTBOX;
}

void DrawTextBox::save(cereal::PortableBinaryOutputArchive& a) const {
    a(d.editing, d.p1, d.p2, textBox->get_rich_text_data(), *cursor);
}

void DrawTextBox::load(cereal::PortableBinaryInputArchive& a) {
    RichTextBox::RichTextData richText;
    a(d.editing, d.p1, d.p2, richText, *cursor);
    textBox->set_rich_text_data(richText);
}

#ifndef IS_SERVER
std::shared_ptr<DrawComponent> DrawTextBox::copy() const {
    auto a = std::make_shared<DrawTextBox>();
    a->d = d;
    a->coords = coords;
    a->textBox->set_rich_text_data(textBox->get_rich_text_data());
    return a;
}

std::shared_ptr<DrawComponent> DrawTextBox::deep_copy() const {
    auto a = std::make_shared<DrawTextBox>();
    a->d = d;
    a->coords = coords;
    a->textBox->set_rich_text_data(textBox->get_rich_text_data());
    a->collisionTree = collisionTree;
    return a;
}

void DrawTextBox::update_from_delayed_ptr(const std::shared_ptr<DrawComponent>& delayedUpdatePtr) {
    std::shared_ptr<DrawTextBox> newPtr = std::static_pointer_cast<DrawTextBox>(delayedUpdatePtr);
    d = newPtr->d;
}

void DrawTextBox::init_text_box(DrawingProgram& drawP) {
    textBox->set_font_collection(drawP.world.main.fonts.collection); // Getting a segfault relating to the paragraph cache means that the font collection hasn't been set yet
    textBox->set_width(d.p2.x() - d.p1.x());
}

void DrawTextBox::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(drawSetupData.shouldDraw) {
        canvas->save();
        canvas_do_calculated_transform(canvas);
        SkRect clipR = SkRect::MakeLTRB(d.p1.x(), d.p1.y(), d.p2.x(), d.p2.y());
        if(d.editing) {
            SkPaint p;
            p.setStyle(SkPaint::kStroke_Style);
            p.setStrokeWidth(0.0f);
            p.setColor4f({0.5f, 0.5f, 0.5f, 1.0f});
            canvas->drawRect(clipR, p);
        }
        canvas->clipRect(clipR);
        canvas->translate(d.p1.x(), d.p1.y());

        RichTextBox::PaintOpts paintOpts;
        paintOpts.cursorColor = {0.7f, 0.7f, 1.0f};
        if(d.editing && cursor)
            paintOpts.cursor = *cursor;

        textBox->paint(canvas, paintOpts);
        canvas->restore();
    }
}

void DrawTextBox::update(DrawingProgram& drawP) {
}

Vector2f DrawTextBox::get_mouse_pos(DrawingProgram& drawP) {
    return coords.get_mouse_pos(drawP.world) - d.p1;
}

void DrawTextBox::initialize_draw_data(DrawingProgram& drawP) {
    init_text_box(drawP);
    create_collider();
}

bool DrawTextBox::collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) {
    return collisionTree.is_collide(checkAgainst);
}

void DrawTextBox::create_collider() {
    using namespace SCollision;
    ColliderCollection<float> strokeObjects;
    std::array<Vector2f, 4> newT = triangle_from_rect_points(d.p1, d.p2);
    strokeObjects.triangle.emplace_back(newT[0], newT[1], newT[2]);
    strokeObjects.triangle.emplace_back(newT[2], newT[3], newT[0]);
    collisionTree.clear();
    collisionTree.calculate_bvh_recursive(strokeObjects);
}

SCollision::AABB<float> DrawTextBox::get_obj_coord_bounds() const {
    return collisionTree.objects.bounds;
}

#endif
