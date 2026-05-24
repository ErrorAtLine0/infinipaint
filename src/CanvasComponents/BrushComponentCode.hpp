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
#include "../CoordSpaceHelper.hpp"
#include <include/core/SkPathBuilder.h>

struct MeshShapeData {
    std::vector<std::vector<Vector2f>> contours;
    template <typename Archive> void serialize(Archive& a) {
        a(contours);
    }
};

namespace BrushComponentCode {
    struct BrushPoint {
        Vector2f pos;
        float width;
        template <typename Archive> void serialize(Archive& a) {
            a(pos, width);
        }
    };

    SkPath brush_stroke_to_skpath(const std::vector<BrushPoint>& brushPoints, bool hasRoundCaps);
    SkPath create_triangles(const std::vector<BrushPoint>& regularPoints, const std::vector<BrushPoint>& smoothedPoints, bool hasRoundCaps);
    std::vector<size_t> get_wedge_indices(const std::vector<BrushPoint>& points);
    std::vector<BrushPoint> smooth_points(const std::vector<BrushPoint>& points, size_t beginIndex, size_t endIndex, unsigned numOfDivisions);
}
