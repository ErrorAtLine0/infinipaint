#include "EllipseCanvasComponent.hpp"
#include "CanvasComponent.hpp"
#include "Helpers/ConvertVec.hpp"
#include "Helpers/SCollision.hpp"
#include <include/core/SkPathBuilder.h>
#include "../DrawCollision.hpp"

CanvasComponentType EllipseCanvasComponent::get_type() const {
    return CanvasComponentType::ELLIPSE;
}

void EllipseCanvasComponent::save(cereal::PortableBinaryOutputArchive& a) const {
    a(d.strokeColor, d.fillColor, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void EllipseCanvasComponent::load(cereal::PortableBinaryInputArchive& a) {
    a(d.strokeColor, d.fillColor, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void EllipseCanvasComponent::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(d.strokeColor, d.fillColor, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void EllipseCanvasComponent::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    a(d.strokeColor, d.fillColor, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void EllipseCanvasComponent::change_stroke_color(const Vector4f& newStrokeColor) {
    d.strokeColor = newStrokeColor;
}

std::optional<Vector4f> EllipseCanvasComponent::get_stroke_color() const {
    return d.strokeColor;
}

void EllipseCanvasComponent::draw(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<void>& predrawData) const {
    SkPaint p;
    if(d.fillStrokeMode == 0 || d.fillStrokeMode == 2) {
        p.setStyle(SkPaint::kFill_Style);
        p.setColor4f(convert_vec4<SkColor4f>(d.fillColor));
        canvas->drawPath(ellipsePath, p);
    }
    if(d.fillStrokeMode == 1 || d.fillStrokeMode == 2) {
        p.setStyle(SkPaint::kStroke_Style);
        p.setColor4f(convert_vec4<SkColor4f>(d.strokeColor));
        p.setStrokeWidth(d.strokeWidth);
        canvas->drawPath(ellipsePath, p);
    }
}

void EllipseCanvasComponent::create_draw_data() {
    SkPathBuilder ellipsePathBuilder;
    SkRect newRect = SkRect::MakeLTRB(d.p1.x(), d.p1.y(), d.p2.x(), d.p2.y());
    ellipsePathBuilder.addOval(newRect);
    ellipsePath = ellipsePathBuilder.detach();
}

void EllipseCanvasComponent::initialize_draw_data(DrawingProgram& drawP) {
    create_draw_data();
    create_collider();
}

std::unique_ptr<CanvasComponent> EllipseCanvasComponent::get_data_copy() const {
    auto toRet = std::make_unique<EllipseCanvasComponent>();
    toRet->d = d;
    return toRet;
}

void EllipseCanvasComponent::set_data_from(const CanvasComponent& other) {
    auto& otherEllipse = static_cast<const EllipseCanvasComponent&>(other);
    d = otherEllipse.d;
}

bool EllipseCanvasComponent::collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const {
    return collisionTree.is_collide(checkAgainst);
}

void EllipseCanvasComponent::create_collider() {
    using namespace SCollision;
    ColliderCollection<float> strokeObjects;

    if(d.fillStrokeMode == 0) {
        Vector2f ellipseCenter = (d.p1 + d.p2) * 0.5;
        float a = ellipseCenter.x() - d.p1.x();
        float b = ellipseCenter.y() - d.p1.y();
        Vector2f prevPoint = ellipseCenter + Vector2f{a * std::cos(0), b * std::sin(0)};
        unsigned numOfSegments = 20;
        float segmentStep = 1.0 / numOfSegments;
        float t = 0.0;
        for(unsigned i = 0; i <= numOfSegments; i++) {
            Vector2f nextPoint = ellipseCenter + Vector2f{a * std::cos(t), b * std::sin(t)};
            strokeObjects.triangle.emplace_back(ellipseCenter, prevPoint, nextPoint);
            prevPoint = nextPoint;
            t += segmentStep * std::numbers::pi * 2.0;
        }
    }
    else if(d.fillStrokeMode == 1) {
        std::vector<Vector2f> points;

        Vector2f ellipseCenter = (d.p1 + d.p2) * 0.5;
        float a = ellipseCenter.x() - d.p1.x();
        float b = ellipseCenter.y() - d.p1.y();
        unsigned numOfSegments = 40;
        float segmentStep = 1.0 / numOfSegments;
        float t = 0.0;
        for(unsigned i = 0; i < numOfSegments; i++) {
            points.emplace_back(ellipseCenter + Vector2f{a * std::cos(t), b * std::sin(t)});
            t += segmentStep * std::numbers::pi * 2.0;
        }
        generate_polyline(strokeObjects, points, d.strokeWidth, true);

    }
    else if(d.fillStrokeMode == 2) {
        float strokeRadius = d.strokeWidth * 0.5f;
        Vector2f wNewP1 = d.p1 - Vector2f{strokeRadius, strokeRadius};
        Vector2f wNewP2 = d.p2 + Vector2f{strokeRadius, strokeRadius};
        Vector2f ellipseCenter = (wNewP1 + wNewP2) * 0.5;
        float a = ellipseCenter.x() - wNewP1.x();
        float b = ellipseCenter.y() - wNewP1.y();
        Vector2f prevPoint = ellipseCenter + Vector2f{a * std::cos(0), b * std::sin(0)};
        unsigned numOfSegments = 20;
        float segmentStep = 1.0 / numOfSegments;
        float t = 0.0;
        for(unsigned i = 0; i <= numOfSegments; i++) {
            Vector2f nextPoint = ellipseCenter + Vector2f{a * std::cos(t), b * std::sin(t)};
            strokeObjects.triangle.emplace_back(ellipseCenter, prevPoint, nextPoint);
            prevPoint = nextPoint;
            t += segmentStep * std::numbers::pi * 2.0;
        }
    }

    collisionTree.clear();
    collisionTree.calculate_bvh_recursive(strokeObjects);
}

SCollision::AABB<float> EllipseCanvasComponent::get_obj_coord_bounds() const {
    return collisionTree.objects.bounds;
}
