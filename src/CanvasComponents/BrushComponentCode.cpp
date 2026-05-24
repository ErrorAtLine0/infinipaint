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

#include "BrushComponentCode.hpp"
#include <Helpers/ConvertVec.hpp>
#include <Helpers/MathExtras.hpp>
#include "../TimePoint.hpp"
#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>

#define DEFAULT_SMOOTHNESS 3

namespace BrushComponentCode {

SkPath brush_stroke_to_skpath(const std::vector<BrushPoint>& brushPoints, bool hasRoundCaps) {
    std::vector<BrushPoint> points = smooth_points(brushPoints, 0, brushPoints.size() - 1, DEFAULT_SMOOTHNESS);
    return create_triangles(brushPoints, points, hasRoundCaps);
}

std::vector<size_t> get_wedge_indices(const std::vector<BrushPoint>& points) {
    if(points.size() < 2)
        return {};
    std::vector<size_t> toRet;
    toRet.emplace_back(0);
    for(size_t i = 1; i < points.size() - 1; i++) {
        Vector2f v1Norm = (points[i - 1].pos - points[i].pos).normalized();
        Vector2f v2Norm = (points[i + 1].pos - points[i].pos).normalized();
        if(v1Norm.dot(v2Norm) > -0.6f)
            toRet.emplace_back(i);
    }
    toRet.emplace_back(points.size() - 1);
    return toRet;
}

std::vector<BrushPoint> smooth_points(const std::vector<BrushPoint>& points, size_t beginIndex, size_t endIndex, unsigned numOfDivisions) {
    size_t pointsSize = endIndex - beginIndex + 1;
    if(pointsSize < 2) 
        return {points[beginIndex], points[endIndex]};

    std::vector<BrushPoint> toRet;

    BrushPoint p0 = points[beginIndex];
    p0.pos = p0.pos + p0.width * (p0.pos - points[beginIndex + 1].pos).normalized();
    BrushPoint p1 = points[beginIndex];
    BrushPoint p2 = points[beginIndex + 1];
    BrushPoint p3;

    for(size_t i = beginIndex; i < endIndex; i++) {

        if(i == endIndex - 1) {
            p3 = points[i + 1];
            p3.pos = p3.pos + p3.width * (p3.pos - points[i].pos).normalized();
        }
        else
            p3 = points[i + 2];

        toRet.emplace_back(p1);

        float tMove = 1.0 / static_cast<float>(numOfDivisions);
        for(unsigned d = 1; d < numOfDivisions; d++) {
            float t = tMove * static_cast<float>(d);
            BrushPoint pToAdd;
            pToAdd.pos = catmull_rom(p0.pos, p1.pos, p2.pos, p3.pos, t);
            pToAdd.width = std::lerp(p1.width, p2.width, t);
            if(!(std::isnan(pToAdd.pos.x()) || std::isnan(pToAdd.pos.y()))) // Don't add NAN values
                toRet.emplace_back(pToAdd);
        }
        p0 = p1;
        p1 = p2;
        p2 = p3;
    }
    toRet.emplace_back(points[endIndex]);

    return toRet;
}

SkPath create_triangles(const std::vector<BrushPoint>& regularPoints, const std::vector<BrushPoint>& smoothedPoints, bool hasRoundCaps) {
    SkPathBuilder pathBuilder;
    const int ARC_SMOOTHNESS = 10;
    const int CIRCLE_SMOOTHNESS = 20;
    const std::vector<BrushPoint>& pointsN = regularPoints;
    std::vector<SkPoint> topPoints;
    std::vector<SkPoint> bottomPoints;

    if(pointsN.size() == 1 && hasRoundCaps) {
        std::vector<Vector2f> circlePoints = gen_circle_points(pointsN.front().pos, pointsN.front().width / 2.0f, CIRCLE_SMOOTHNESS);
        for(size_t i = 0; i < circlePoints.size(); i++) {
            topPoints.emplace_back(convert_vec2<SkPoint>(circlePoints[i]));
        }
    }

    if(pointsN.size() < 2) {
        if(!topPoints.empty()) {
            pathBuilder.addPolygon({topPoints.data(), topPoints.size()}, true);
            return pathBuilder.detach();
        }
        return SkPath();
    }

    auto& points = smoothedPoints;

    std::vector<size_t> wedgeIndices = get_wedge_indices(points);
    for(size_t i = 0; i < wedgeIndices.size() - 1; i++) {

        size_t pointsBegin = wedgeIndices[i];
        size_t pointsEnd = wedgeIndices[i + 1];

        std::array<Vector2f, 3> vertArray;
        size_t newIndex = 0;

        Vector2f perpPrev = perpendicular_vec2((points[pointsBegin + 1].pos - points[pointsBegin].pos).eval());
        perpPrev.normalize();

        vertArray[0] = points[pointsBegin].pos + perpPrev * points[pointsBegin].width * 0.5f;
        topPoints.emplace_back(convert_vec2<SkPoint>(vertArray[0]));
        vertArray[1] = points[pointsBegin].pos - perpPrev * points[pointsBegin].width * 0.5f;
        bottomPoints.emplace_back(convert_vec2<SkPoint>(vertArray[1]));
        newIndex = 2;

        if(hasRoundCaps && pointsBegin == 0) {
            Vector2f arcDirStart = (convert_vec2<Vector2f>(vertArray[0]) - points[0].pos).normalized();
            Vector2f arcDirEnd = (convert_vec2<Vector2f>(vertArray[1]) - points[0].pos).normalized();
            Vector2f arcDirCenter = (points[0].pos - points[1].pos).normalized();
            std::vector<Vector2f> arcPoints = arc_vec<float>(points[0].pos, arcDirStart, arcDirEnd, arcDirCenter, points[0].width * 0.5f, ARC_SMOOTHNESS);
            arcPoints.emplace(arcPoints.begin(), convert_vec2<Vector2f>(vertArray[0]));
            arcPoints.emplace_back(convert_vec2<Vector2f>(vertArray[1]));
            for(size_t i = 0; i < arcPoints.size() - 1; i++) {
                bottomPoints.emplace_back(convert_vec2<SkPoint>(arcPoints[i]));
            }
            bottomPoints.emplace_back(convert_vec2<SkPoint>(arcPoints.back()));
        }

        for(size_t j = pointsBegin + 1; j < pointsEnd; j++) {
            if(j >= pointsEnd)
                j = pointsEnd - 1;

            Vector2f v1 = points[j - 1].pos - points[j].pos;
            Vector2f v2 = points[j].pos - points[j + 1].pos;
            Vector2f perp = perpendicular_vec2((v1 + v2).eval()).normalized();
            if(std::isnan(perp.dot(perpPrev))) {
                std::cout << "NAN FOUND" << std::endl;
                std::cout << v1 << std::endl;
                std::cout << v2 << std::endl;
                std::cout << perp << std::endl;
                std::cout << perpPrev << std::endl;
                throw 1;
            }
            if(perp.dot(perpPrev) < 0.0)
                perp = -perp;

            vertArray[newIndex] = points[j].pos + perp * 0.5f * points[j].width;
            topPoints.emplace_back(convert_vec2<SkPoint>(vertArray[newIndex]));
            newIndex = (newIndex + 1) % 3;
            
            vertArray[newIndex] = points[j].pos - perp * 0.5f * points[j].width;
            bottomPoints.emplace_back(convert_vec2<SkPoint>(vertArray[newIndex]));
            newIndex = (newIndex + 1) % 3;

            perpPrev = perp;
        }

        Vector2f rectDir = points[pointsEnd - 1].pos - points[pointsEnd].pos;
        rectDir.normalize();
        Vector2f perp = perpendicular_vec2(rectDir);
        if(perp.dot(perpPrev) < 0.0)
            perp = -perp;
        vertArray[newIndex] = points[pointsEnd].pos + perp * points[pointsEnd].width * 0.5;
        topPoints.emplace_back(convert_vec2<SkPoint>(vertArray[newIndex]));
        newIndex = (newIndex + 1) % 3;

        vertArray[newIndex] = points[pointsEnd].pos - perp * points[pointsEnd].width * 0.5;
        bottomPoints.emplace_back(convert_vec2<SkPoint>(vertArray[newIndex]));
        newIndex = (newIndex + 1) % 3;

        // Render wedge
        if(pointsEnd != points.size() - 1) {
            // Find 4 points involved in intersection
            Vector2f currentP1 = points[pointsEnd].pos - perp * points[pointsEnd].width * 0.5;
            Vector2f currentP2 = points[pointsEnd].pos + perp * points[pointsEnd].width * 0.5;
            Vector2f nextRectDir = points[pointsEnd + 1].pos - points[pointsEnd].pos;
            nextRectDir.normalize();
            Vector2f nextPerp = perpendicular_vec2(nextRectDir);
            Vector2f nextP1 = points[pointsEnd].pos - nextPerp * points[pointsEnd].width * 0.5;
            Vector2f nextP2 = points[pointsEnd].pos + nextPerp * points[pointsEnd].width * 0.5;

            // Find center of wedge
            Vector2f wedgeCenter = line_line_intersection(currentP1, currentP2, nextP1, nextP2);

            Vector2f wedgeP1 = ((currentP1 - currentP2).dot(nextRectDir)) <= 0.0 ? currentP1 : currentP2;
            Vector2f wedgeP2 = ((nextP1 - nextP2).dot(rectDir)) <= 0.0 ? nextP1 : nextP2;
            bool isTopWedge = wedgeP1 == convert_vec2<Vector2f>(topPoints.back());

            float arcRadius;

            std::vector<Vector2f> arcPoints;

            if(wedgeCenter.x() == std::numeric_limits<float>::max()) {
                wedgeCenter = (currentP1 + currentP2) * 0.5;
                wedgeP1 = currentP1;
                wedgeP2 = currentP2;
                arcRadius = points[pointsEnd].width * 0.5;
                arcPoints = arc_vec(wedgeCenter, (wedgeP1 - wedgeCenter).normalized(), (wedgeP2 - wedgeCenter).normalized(), (-rectDir).eval(), arcRadius, ARC_SMOOTHNESS);
            }
            else {
                arcRadius = (wedgeCenter - wedgeP1).norm();
                arcPoints = arc_vec(wedgeCenter, (wedgeP1 - wedgeCenter).normalized(), (wedgeP2 - wedgeCenter).normalized(), lerp_vec((-rectDir).eval(), (-nextRectDir).eval(), 0.5).normalized(), arcRadius, ARC_SMOOTHNESS);
            }

            arcPoints.emplace(arcPoints.begin(), wedgeP1);
            arcPoints.emplace_back(wedgeP2);
            for(size_t i = 0; i < arcPoints.size() - 1; i++) {
                if(isTopWedge)
                    topPoints.emplace_back(convert_vec2<SkPoint>(arcPoints[i]));
                else
                    bottomPoints.emplace_back(convert_vec2<SkPoint>(arcPoints[i]));
            }
            if(isTopWedge)
                topPoints.emplace_back(convert_vec2<SkPoint>(arcPoints.back()));
            else
                bottomPoints.emplace_back(convert_vec2<SkPoint>(arcPoints.back()));
        }
        else if(hasRoundCaps) { // It's the last point, cap it off
            Vector2f arcDirStart = (convert_vec2<Vector2f>(vertArray[(newIndex + 1) % 3]) - points.back().pos).normalized();
            Vector2f arcDirEnd = (convert_vec2<Vector2f>(vertArray[(newIndex + 2) % 3]) - points.back().pos).normalized();
            Vector2f arcDirCenter = -rectDir;
            std::vector<Vector2f> arcPoints = arc_vec<float>(points.back().pos, arcDirStart, arcDirEnd, arcDirCenter, points.back().width * 0.5f, ARC_SMOOTHNESS);
            arcPoints.emplace(arcPoints.begin(), convert_vec2<Vector2f>(vertArray[(newIndex + 1) % 3]));
            arcPoints.emplace_back(convert_vec2<Vector2f>(vertArray[(newIndex + 2) % 3]));
            for(size_t i = 0; i < arcPoints.size() - 1; i++) {
                topPoints.emplace_back(convert_vec2<SkPoint>(arcPoints[i]));
            }
            topPoints.emplace_back(convert_vec2<SkPoint>(arcPoints.back()));
        }
    }

    if(!topPoints.empty()) {
        topPoints.insert(topPoints.begin(), bottomPoints.rbegin(), bottomPoints.rend());
        pathBuilder.addPolygon({topPoints.data(), topPoints.size()}, true);
    }

    return pathBuilder.detach();
}

}
