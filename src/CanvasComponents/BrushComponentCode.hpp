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
#include "../InputManager.hpp"

class DrawingProgram;

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

    struct BrushStrokeGenerationData {
        bool addedTemporaryPoint = false;
        std::vector<BrushComponentCode::BrushPoint> brushPoints;
        Vector2f prevPointUnaltered = {0, 0};
        float penWidth = 1.0f;
        CoordSpaceHelper coords;
    };

    SkPath brush_stroke_to_skpath(const std::vector<BrushPoint>& brushPoints, bool hasRoundCaps);
    SkPath create_triangles(const std::vector<BrushPoint>& regularPoints, const std::vector<BrushPoint>& smoothedPoints, bool hasRoundCaps);
    std::vector<size_t> get_wedge_indices(const std::vector<BrushPoint>& points);
    std::vector<BrushPoint> smooth_points(const std::vector<BrushPoint>& points, size_t beginIndex, size_t endIndex, unsigned numOfDivisions);
    void smooth_out_points(std::vector<BrushPoint>& brushPoints, float smoothFactor);
    void fix_tip(std::vector<BrushPoint>& brushPoints);
    void mouse_button(DrawingProgram& drawP, BrushStrokeGenerationData& genData, const CoordSpaceHelper& strokeCoordSpace, const InputManager::MouseButtonCallbackArgs& button, float brushSize);
    void mouse_motion(DrawingProgram& drawP, BrushStrokeGenerationData& genData, const Vector2f& motionPos, float brushSize);
    void pen_pressure(DrawingProgram& drawP, BrushStrokeGenerationData& genData, float brushSize);
    bool extensive_point_checking(const std::vector<BrushPoint>& points, const Vector2f& newPoint, float minimumDistance);
    bool extensive_point_checking_back(const std::vector<BrushPoint>& points, const Vector2f& newPoint);

}
