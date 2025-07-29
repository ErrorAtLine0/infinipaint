#include "DrawTextBox.hpp"
#include "Helpers/ConvertVec.hpp"
#include <cereal/types/string.hpp>

#ifndef IS_SERVER
#include "../MainProgram.hpp"
#include "Helpers/SCollision.hpp"
#include "../DrawingProgram/DrawingProgram.hpp"
#include "../DrawCollision.hpp"
#endif

DrawComponentType DrawTextBox::get_type() const {
    return DRAWCOMPONENT_TEXTBOX;
}

void DrawTextBox::save(cereal::PortableBinaryOutputArchive& a) const {
    a(d.editing, d.p1, d.p2, d.textColor, d.textSize, d.cursor, d.currentText);
}

void DrawTextBox::load(cereal::PortableBinaryInputArchive& a) {
    a(d.editing, d.p1, d.p2, d.textColor, d.textSize, d.cursor, d.currentText);
#ifndef IS_SERVER
    textboxUpdate = true;
#endif
}

#ifndef IS_SERVER
std::shared_ptr<DrawComponent> DrawTextBox::copy() const {
    auto a = std::make_shared<DrawTextBox>();
    a->d = d;
    a->coords = coords;
    a->textboxUpdate = true;
    return a;
}

std::shared_ptr<DrawComponent> DrawTextBox::deep_copy() const {
    auto a = std::make_shared<DrawTextBox>();
    a->d = d;
    a->coords = coords;
    a->textboxUpdate = false;
    a->collisionTree = collisionTree;
    a->textBox = textBox;
    return a;
}

void DrawTextBox::update_from_delayed_ptr() {
    std::shared_ptr<DrawTextBox> newPtr = std::static_pointer_cast<DrawTextBox>(delayedUpdatePtr);
    d = newPtr->d;
    textboxUpdate = true;
}

void DrawTextBox::init_text_box(DrawingProgram& drawP) {
    textBox.setFontMgr(drawP.world.main.fonts.mgr);
    SkFont f(drawP.world.main.fonts.map["Roboto"], d.textSize);
    f.setLinearMetrics(true);
    f.setHinting(SkFontHinting::kNormal);
    //font.setForceAutoHinting(true);
    f.setSubpixel(true);
    f.setBaselineSnap(true);
    f.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    textBox.setFont(f);
    textBox.setWidth(d.p2.x() - d.p1.x());
    textBox.fLines.clear();
    textBox.insert({0, 0}, d.currentText);
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

        CollabTextBox::Editor::PaintOpts paintOpts;
        paintOpts.fForegroundColor = convert_vec4<SkColor4f>(d.textColor);
        paintOpts.fBackgroundColor = {0.0f, 0.0f, 0.0f, 0.0f};
        paintOpts.cursorColor = {0.2f, 0.2f, 0.7f, 1.0f};
        paintOpts.showCursor = d.editing;
        paintOpts.cursor = d.cursor;

        textBox.paint(canvas, paintOpts);
        canvas->restore();
    }
}

void DrawTextBox::update(DrawingProgram& drawP) {
    if(textboxUpdate)
        set_textbox_string(d.currentText);
}

void DrawTextBox::set_textbox_string(const std::string& str) {
    d.currentText = str;
    textBox.fLines.clear();
    textBox.insert({0, 0}, d.currentText);
    textboxUpdate = false;
}

void DrawTextBox::update_contained_string(DrawingProgram& drawP) {
    d.currentText = textBox.get_string();
    temp_update(drawP);
}

Vector2f DrawTextBox::get_mouse_pos(DrawingProgram& drawP) {
    return coords.get_mouse_pos(drawP.world) - d.p1;
}

void DrawTextBox::initialize_draw_data(DrawingProgram& drawP) {
    init_text_box(drawP);
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
