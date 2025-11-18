#include "DrawRectangle.hpp"
#include "Helpers/ConvertVec.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include <include/core/SkPaint.h>
#include <include/core/SkPathBuilder.h>
#include "../SharedTypes.hpp"

#ifndef IS_SERVER
#include "../DrawCollision.hpp"
#endif

DrawComponentType DrawRectangle::get_type() const {
    return DRAWCOMPONENT_RECTANGLE;
}

void DrawRectangle::save(cereal::PortableBinaryOutputArchive& a) const {
    a(d.strokeColor, d.fillColor, d.cornerRadius, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void DrawRectangle::load(cereal::PortableBinaryInputArchive& a) {
    a(d.strokeColor, d.fillColor, d.cornerRadius, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void DrawRectangle::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(d.strokeColor, d.fillColor, d.cornerRadius, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void DrawRectangle::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    a(d.strokeColor, d.fillColor, d.cornerRadius, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

#ifndef IS_SERVER
std::shared_ptr<DrawComponent> DrawRectangle::copy(DrawingProgram& drawP) const {
    auto a = std::make_shared<DrawRectangle>();
    a->d = d;
    a->coords = coords;
    return a;
}

std::shared_ptr<DrawComponent> DrawRectangle::deep_copy(DrawingProgram& drawP) const {
    auto a = std::make_shared<DrawRectangle>();
    a->d = d;
    a->coords = coords;
    a->rectPath = rectPath;
    a->collisionTree = collisionTree;
    return a;
}

void DrawRectangle::update_from_delayed_ptr(const std::shared_ptr<DrawComponent>& delayedUpdatePtr) {
    std::shared_ptr<DrawRectangle> newPtr = std::static_pointer_cast<DrawRectangle>(delayedUpdatePtr);
    d = newPtr->d;
}

void DrawRectangle::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(drawSetupData.shouldDraw) {
        canvas->save();
        canvas_do_calculated_transform(canvas);
        if(d.fillStrokeMode == 0 || d.fillStrokeMode == 2) {
            SkPaint p;
            p.setStyle(SkPaint::kFill_Style);
            p.setStrokeCap(SkPaint::kRound_Cap);
            p.setColor4f(convert_vec4<SkColor4f>(d.fillColor));
            canvas->drawPath(rectPath, p);
        }
        if(d.fillStrokeMode == 1 || d.fillStrokeMode == 2) {
            SkPaint p;
            p.setStyle(SkPaint::kStroke_Style);
            p.setStrokeCap(SkPaint::kRound_Cap);
            p.setColor4f(convert_vec4<SkColor4f>(d.strokeColor));
            p.setStrokeWidth(d.strokeWidth);
            canvas->drawPath(rectPath, p);
        }
        canvas->restore();
    }
}

void DrawRectangle::update(DrawingProgram& drawP) {
}

void DrawRectangle::initialize_draw_data(DrawingProgram& drawP) {
    create_draw_data();
    create_collider();
}

bool DrawRectangle::collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) {
    return collisionTree.is_collide(checkAgainst);
}

void DrawRectangle::create_draw_data() {
    SkPathBuilder rectPathBuilder;
    if(d.p1.x() == d.p2.x() || d.p1.y() == d.p2.y()) {
        rectPathBuilder.moveTo(convert_vec2<SkPoint>(d.p1));
        rectPathBuilder.lineTo(convert_vec2<SkPoint>(d.p2));
        rectPath = rectPathBuilder.detach();
    }
    else {
        SkRRect newRect = SkRRect::MakeRectXY(SkRect::MakeLTRB(d.p1.x(), d.p1.y(), d.p2.x(), d.p2.y()), d.cornerRadius, d.cornerRadius);
        rectPathBuilder.addRRect(newRect);
        rectPath = rectPathBuilder.detach();
    }
}

void DrawRectangle::create_collider() {
    using namespace SCollision;
    ColliderCollection<float> strokeObjects;
    if(d.fillStrokeMode == 0) {
        std::array<Vector2f, 4> newT = triangle_from_rect_points(d.p1, d.p2);
        strokeObjects.triangle.emplace_back(newT[0], newT[1], newT[2]);
        strokeObjects.triangle.emplace_back(newT[2], newT[3], newT[0]);
    }
    else if(d.fillStrokeMode == 1) {
        if(d.p1.x() == d.p2.x() || d.p1.y() == d.p2.y()) {
            std::vector<Vector2f> points = {d.p1, d.p2};
            generate_polyline(strokeObjects, points, d.strokeWidth, true);
        }
        else {
            std::array<Vector2f, 4> pointArr = triangle_from_rect_points(d.p1, d.p2);
            std::vector<Vector2f> points(pointArr.begin(), pointArr.end());
            generate_polyline(strokeObjects, points, d.strokeWidth, true);
        }
    }
    else if(d.fillStrokeMode == 2) {
        float strokeRadius = d.strokeWidth * 0.5f;
        std::array<Vector2f, 4> newT = triangle_from_rect_points((d.p1 - Vector2f{strokeRadius, strokeRadius}).eval(), (d.p2 + Vector2f{strokeRadius, strokeRadius}).eval());
        strokeObjects.triangle.emplace_back(newT[0], newT[1], newT[2]);
        strokeObjects.triangle.emplace_back(newT[2], newT[3], newT[0]);
    }

    collisionTree.clear();
    collisionTree.calculate_bvh_recursive(strokeObjects);
}

SCollision::AABB<float> DrawRectangle::get_obj_coord_bounds() const {
    return collisionTree.objects.bounds;
}

#endif
