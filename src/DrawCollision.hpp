#pragma once
#include <Helpers/SCollision.hpp>
#include "DrawData.hpp"
#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>

void draw_collider(SkCanvas* canvas, const DrawData& drawData, const SCollision::AABB<float>& aabb, const SkPaint& p);
void draw_collider(SkCanvas* canvas, const DrawData& drawData, const SCollision::Circle<float>& circle, const SkPaint& p);
void draw_collider(SkCanvas* canvas, const DrawData& drawData, const SCollision::Triangle<float>& triangle, const SkPaint& p);
void draw_collider(SkCanvas* canvas, const DrawData& drawData, const SCollision::ColliderCollection<float>& collection, const SkPaint& p);
void draw_collider(SkCanvas* canvas, const DrawData& drawData, const SCollision::BVHContainer<float>& bvh, const SkPaint& p);

