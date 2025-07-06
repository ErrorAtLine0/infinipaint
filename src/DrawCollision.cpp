#include "DrawCollision.hpp"
#include <include/core/SkPaint.h>
#include <include/core/SkPath.h>

void draw_collider(SkCanvas* canvas, const DrawData& drawData, const SCollision::AABB<float>& aabb, const SkPaint& p) {
    SkRect r = SkRect::MakeLTRB(aabb.min.x(), aabb.min.y(), aabb.max.x(), aabb.max.y());
    canvas->drawRect(r, p);
}

void draw_collider(SkCanvas* canvas, const DrawData& drawData, const SCollision::Circle<float>& circle, const SkPaint& p) {
    canvas->drawCircle(circle.pos.x(), circle.pos.y(), circle.radius, p);
}

void draw_collider(SkCanvas* canvas, const DrawData& drawData, const SCollision::Triangle<float>& triangle, const SkPaint& p) {
    SkPath path;
    path.moveTo(convert_vec2<SkPoint>(triangle.p[0]));
    path.lineTo(convert_vec2<SkPoint>(triangle.p[1]));
    path.lineTo(convert_vec2<SkPoint>(triangle.p[2]));
    path.close();
    canvas->drawPath(path, p);
}

void draw_collider(SkCanvas* canvas, const DrawData& drawData, const SCollision::ColliderCollection<float>& collection, const SkPaint& p) {
    for(auto& aabb : collection.aabb)
        draw_collider(canvas, drawData, aabb, p);
    for(auto& circle : collection.circle)
        draw_collider(canvas, drawData, circle, p);
    for(auto& triangle : collection.triangle)
        draw_collider(canvas, drawData, triangle, p);
}

void draw_collider(SkCanvas* canvas, const DrawData& drawData, const SCollision::BVHContainer<float>& bvh, const SkPaint& p) {
    for(auto& child : bvh.children)
        draw_collider(canvas, drawData, child, p);
    draw_collider(canvas, drawData, bvh.objects, p);
}
