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

