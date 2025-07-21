#include "DrawBrushStroke.hpp"
#include <Helpers/Serializers.hpp>
#include <cereal/types/vector.hpp>
#include <limits>

#ifndef IS_SERVER
#include <Eigen/Geometry>
#include "Helpers/MathExtras.hpp"
#include "../TimePoint.hpp"
#include <include/core/SkVertices.h>
#include "../DrawCollision.hpp"
#endif

#define AABB_PRECHECK_BVH_LEVELS 2
#define DEFAULT_SMOOTHNESS 5

void DrawBrushStroke::save(cereal::PortableBinaryOutputArchive& a) const {
    a(d.points, d.color, d.hasRoundCaps);
}

void DrawBrushStroke::load(cereal::PortableBinaryInputArchive& a) {
    a(d.points, d.color, d.hasRoundCaps);
}

DrawComponentType DrawBrushStroke::get_type() const {
    return DRAWCOMPONENT_BRUSHSTROKE;
}

#ifndef IS_SERVER

std::shared_ptr<DrawComponent> DrawBrushStroke::copy() const {
    auto a = std::make_shared<DrawBrushStroke>();
    a->d = d;
    a->coords = coords;
    return a;
}

void DrawBrushStroke::update_from_delayed_ptr() {
    std::shared_ptr<DrawBrushStroke> newPtr = std::static_pointer_cast<DrawBrushStroke>(delayedUpdatePtr);
    d = newPtr->d;
}

Vector2f two_dim_vec_slerp(const Vector2f& v1, const Vector2f& v2) {
    Vector2f v1Norm = v1.normalized();
    Vector2f v2Norm = v2.normalized();
    Vector2f avg = (v1Norm + v2Norm) * 0.5f;
    if(std::fabs(avg.x()) < 0.00001f && std::fabs(avg.y()) < 0.00001f)
        return perpendicular_vec2(v1Norm).normalized();
    else
        return avg.normalized();
}

void DrawBrushStroke::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(drawSetupData.shouldDraw && vertices) {
        //bool deepPrecheck = false;
        //for(auto& aabb : precheckAABBLevels) {
        //    if(SCollision::collide(drawData.cam.viewingAreaGenerousCollider, aabb)) {
        //        deepPrecheck = true;
        //        break;
        //    }
        //}
        //if(!deepPrecheck && !precheckAABBLevels.empty())
        //    return;

        SkPaint paint;
        if(d.color.w() == 1.0) // Fully opaque, dont bother with blending
            canvas->save();
        else
            canvas->saveLayerAlphaf(nullptr, d.color.w());
        paint.setColor4f(SkColor4f{d.color.x(), d.color.y(), d.color.z(), 1.0f});
        canvas_do_calculated_transform(canvas);
        if(drawSetupData.mipmapLevel == 0)
            canvas->drawVertices(vertices, SkBlendMode::kClear, paint);
        else
            canvas->drawVertices(verticesMipMap[drawSetupData.mipmapLevel - 1], SkBlendMode::kClear, paint);
        //SkPaint p2(SkColor4f{1, 0, 0, 1});
        //p2.setStroke(true);
        //SkPaint p3(SkColor4f{0, 1, 0, 1});
        //SkPaint p4(SkColor4f{0, 0, 1, 1});
        //p3.setStroke(true);
        //p4.setStroke(true);
        //for(auto& precheckAABBLevel : precheckAABBLevels)
        //    draw_collider(canvas, drawData, precheckAABBLevel, p4);
        //draw_collider(canvas, drawData, collisionTree, p3);
        canvas->restore();
    }
}

void DrawBrushStroke::initialize_draw_data(DrawingProgram& drawP) {
    if(!d.points.empty()) {
        std::vector<SkPoint> vertexData;
        std::vector<DrawBrushStrokePoint> points = smooth_points(0, d.points.size() - 1, DEFAULT_SMOOTHNESS);
        auto triangleFunc = [&](Vector2f a, Vector2f b, Vector2f c) {
            vertexData.emplace_back(a.x(), a.y());
            vertexData.emplace_back(b.x(), b.y());
            vertexData.emplace_back(c.x(), c.y());
            return false;
        };

        create_triangles(triangleFunc, points, 0);
        vertices_to_draw_data(vertices, vertexData);
        vertexData.clear();

        create_triangles(triangleFunc, points, 4);
        vertices_to_draw_data(verticesMipMap[0], vertexData);
        vertexData.clear();

        create_triangles(triangleFunc, points, 30);
        vertices_to_draw_data(verticesMipMap[1], vertexData);
    }
}

std::vector<size_t> DrawBrushStroke::get_wedge_indices(const std::vector<DrawBrushStrokePoint>& points) {
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

std::vector<DrawBrushStrokePoint> DrawBrushStroke::smooth_points(size_t beginIndex, size_t endIndex, unsigned numOfDivisions) {
    std::vector<DrawBrushStrokePoint>& points = d.points;

    size_t pointsSize = endIndex - beginIndex + 1;
    if(pointsSize < 2) 
        return {points[beginIndex], points[endIndex]};

    std::vector<DrawBrushStrokePoint> toRet;

    DrawBrushStrokePoint p0 = points[beginIndex];
    p0.pos = p0.pos + p0.width * (p0.pos - points[beginIndex + 1].pos).normalized() * DRAW_MINIMUM_LIMIT;
    DrawBrushStrokePoint p1 = points[beginIndex];
    DrawBrushStrokePoint p2 = points[beginIndex + 1];
    DrawBrushStrokePoint p3;

    for(size_t i = beginIndex; i < endIndex; i++) {

        if(i == endIndex - 1) {
            p3 = points[i + 1];
            p3.pos = p3.pos + p3.width * (p3.pos - points[i].pos).normalized() * DRAW_MINIMUM_LIMIT;
        }
        else
            p3 = points[i + 2];

        toRet.emplace_back(p1);

        float tMove = 1.0 / static_cast<float>(numOfDivisions);
        for(unsigned d = 1; d < numOfDivisions; d++) {
            float t = tMove * static_cast<float>(d);
            DrawBrushStrokePoint pToAdd;
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

std::vector<DrawBrushStrokePoint> DrawBrushStroke::smooth_points_avg(size_t beginIndex, size_t endIndex, unsigned numOfDivisions) {
    std::vector<DrawBrushStrokePoint>& points = d.points;

    size_t pointsSize = endIndex - beginIndex + 1;
    if(pointsSize < 2) 
        return {points[beginIndex], points[endIndex]};

    std::vector<DrawBrushStrokePoint> toRet;

    for(size_t i = beginIndex; i < endIndex; i++) {
        const DrawBrushStrokePoint& p1 = points[i];
        const DrawBrushStrokePoint& p2 = points[i + 1];

        float tMove = 1.0 / static_cast<float>(numOfDivisions);
        for(unsigned d = 0; d < numOfDivisions; d++) {
            float t = tMove * static_cast<float>(d);
            DrawBrushStrokePoint pToAdd;
            pToAdd.pos = lerp_vec(p1.pos, p2.pos, t);
            pToAdd.width = std::lerp(p1.width, p2.width, t);
            if(!(std::isnan(pToAdd.pos.x()) || std::isnan(pToAdd.pos.y()))) // Don't add NAN values
                toRet.emplace_back(pToAdd);
        }
    }

    toRet.emplace_back(points[endIndex]);

    return toRet;
}

void DrawBrushStroke::create_triangles(const std::function<bool(Vector2f, Vector2f, Vector2f)>& passTriangleFunc, const std::vector<DrawBrushStrokePoint>& points, unsigned skipVertexCount) {
    const int ARC_SMOOTHNESS = 10;
    const int CIRCLE_SMOOTHNESS = 20;
    std::vector<DrawBrushStrokePoint>& pointsN = d.points;

    if(pointsN.size() == 1 && d.hasRoundCaps) {
        std::vector<Vector2f> circlePoints = gen_circle_points(pointsN.front().pos, pointsN.front().width / 2.0f, CIRCLE_SMOOTHNESS);
        for(size_t i = 0; i < circlePoints.size(); i++)
            if(passTriangleFunc(pointsN.front().pos, circlePoints[i], circlePoints[(i + 1) % circlePoints.size()])) return;
    }

    if(pointsN.size() < 2)
        return;

    std::vector<size_t> wedgeIndices = get_wedge_indices(points);
    for(size_t i = 0; i < wedgeIndices.size() - 1; i++) {

        size_t pointsBegin = wedgeIndices[i];
        size_t pointsEnd = wedgeIndices[i + 1];

        std::array<Vector2f, 3> vertArray;
        size_t newIndex = 0;

        Vector2f perpPrev = perpendicular_vec2((points[pointsBegin + 1].pos - points[pointsBegin].pos).eval());
        perpPrev.normalize();

        vertArray[0] = points[pointsBegin].pos + perpPrev * points[pointsBegin].width * 0.5f;
        vertArray[1] = points[pointsBegin].pos - perpPrev * points[pointsBegin].width * 0.5f;
        newIndex = 2;

        if(d.hasRoundCaps && pointsBegin == 0) {
            Vector2f arcDirStart = (convert_vec2<Vector2f>(vertArray[0]) - points[0].pos).normalized();
            Vector2f arcDirEnd = (convert_vec2<Vector2f>(vertArray[1]) - points[0].pos).normalized();
            Vector2f arcDirCenter = (points[0].pos - points[1].pos).normalized();
            std::vector<Vector2f> arcPoints = arc_vec<float>(points[0].pos, arcDirStart, arcDirEnd, arcDirCenter, points[0].width * 0.5f, ARC_SMOOTHNESS);
            arcPoints.emplace(arcPoints.begin(), convert_vec2<Vector2f>(vertArray[0]));
            arcPoints.emplace_back(convert_vec2<Vector2f>(vertArray[1]));
            for(size_t i = 0; i < arcPoints.size() - 1; i++)
                if(passTriangleFunc(points[0].pos, arcPoints[i], arcPoints[i + 1])) return;
        }

        for(size_t j = pointsBegin + 1; j < pointsEnd; j++) {
            j += skipVertexCount;
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
            newIndex = (newIndex + 1) % 3;
            if(passTriangleFunc(vertArray[0], vertArray[1], vertArray[2])) return;
            
            vertArray[newIndex] = points[j].pos - perp * 0.5f * points[j].width;
            newIndex = (newIndex + 1) % 3;
            if(passTriangleFunc(vertArray[0], vertArray[1], vertArray[2])) return;

            perpPrev = perp;
        }

        Vector2f rectDir = points[pointsEnd - 1].pos - points[pointsEnd].pos;
        rectDir.normalize();
        Vector2f perp = perpendicular_vec2(rectDir);
        if(perp.dot(perpPrev) < 0.0)
            perp = -perp;
        vertArray[newIndex] = points[pointsEnd].pos + perp * points[pointsEnd].width * 0.5;
        newIndex = (newIndex + 1) % 3;
        if(passTriangleFunc(vertArray[0], vertArray[1], vertArray[2])) return;

        vertArray[newIndex] = points[pointsEnd].pos - perp * points[pointsEnd].width * 0.5;
        newIndex = (newIndex + 1) % 3;
        if(passTriangleFunc(vertArray[0], vertArray[1], vertArray[2])) return;

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
            for(size_t i = 0; i < arcPoints.size() - 1; i++)
                if(passTriangleFunc(wedgeCenter, arcPoints[i], arcPoints[i + 1])) return;
        }
        else if(d.hasRoundCaps) { // It's the last point, cap it off
            Vector2f arcDirStart = (convert_vec2<Vector2f>(vertArray[(newIndex + 1) % 3]) - points.back().pos).normalized();
            Vector2f arcDirEnd = (convert_vec2<Vector2f>(vertArray[(newIndex + 2) % 3]) - points.back().pos).normalized();
            Vector2f arcDirCenter = -rectDir;
            std::vector<Vector2f> arcPoints = arc_vec<float>(points.back().pos, arcDirStart, arcDirEnd, arcDirCenter, points.back().width * 0.5f, ARC_SMOOTHNESS);
            arcPoints.emplace(arcPoints.begin(), convert_vec2<Vector2f>(vertArray[(newIndex + 1) % 3]));
            arcPoints.emplace_back(convert_vec2<Vector2f>(vertArray[(newIndex + 2) % 3]));
            for(size_t i = 0; i < arcPoints.size() - 1; i++)
                if(passTriangleFunc(points.back().pos, arcPoints[i], arcPoints[i + 1])) return;
        }
    }
}

void DrawBrushStroke::create_collider(bool colliderAllocated) {
    using namespace SCollision;
    if(d.points.empty())
        return;
    ColliderCollection<float> strokeObjects;
    std::vector<DrawBrushStrokePoint> points = smooth_points(0, d.points.size() - 1, DEFAULT_SMOOTHNESS);
    create_triangles([&](Vector2f a, Vector2f b, Vector2f c) {
        strokeObjects.triangle.emplace_back(a, b, c);
        return false;
    }, points, 0);
    strokeObjects.recalculate_bounds();
    collisionTree.clear();
    collisionTree.calculate_bvh_recursive(strokeObjects);
    precheckAABBLevels.clear();
    add_precheck_aabb_level(0, collisionTree.children);
    if(!colliderAllocated) {
        collisionTree.objects.aabb.clear();
        collisionTree.objects.circle.clear();
        collisionTree.objects.triangle.clear();
        collisionTree.children.clear();
    }
    calculate_world_bounds();
}

void DrawBrushStroke::add_precheck_aabb_level(size_t level, const std::vector<SCollision::BVHContainer<float>>& levelArray) {
    for(const auto& a : levelArray) {
        if(level == AABB_PRECHECK_BVH_LEVELS || a.children.empty())
            precheckAABBLevels.emplace_back(a.objects.bounds);
        else
            add_precheck_aabb_level(level + 1, a.children);
    }
}

void DrawBrushStroke::vertices_to_draw_data(sk_sp<SkVertices>& vertexDataPtr, const std::vector<SkPoint>& vertexData) {
    if((d.points.size() == 1 && d.hasRoundCaps) || d.points.size() >= 2)
        vertexDataPtr = SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode, vertexData.size(), vertexData.data(), nullptr, nullptr);
    else
        vertexDataPtr = SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode, 0, nullptr, nullptr, nullptr);
}

void DrawBrushStroke::update(DrawingProgram& drawP) {
}

bool DrawBrushStroke::collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst, bool colliderAllocated) {
    if(colliderAllocated)
        return collisionTree.is_collide(checkAgainst);

    if(!SCollision::collide(checkAgainst, collisionTree.objects.bounds))
        return false;

    bool deepPrecheck = false;
    for(auto& aabb : precheckAABBLevels) {
        if(SCollision::collide(checkAgainst, aabb)) {
            deepPrecheck = true;
            break;
        }
    }
    if(!deepPrecheck && !precheckAABBLevels.empty())
        return false;

    bool toRet = false;
    std::vector<DrawBrushStrokePoint> points = smooth_points(0, d.points.size() - 1, DEFAULT_SMOOTHNESS);
    create_triangles([&](Vector2f a, Vector2f b, Vector2f c) {
        toRet = SCollision::collide(checkAgainst, SCollision::Triangle<float>{a, b, c});
        return toRet;
    }, points, 0);
    return toRet;
}

void DrawBrushStroke::free_collider() {
    collisionTree.objects.aabb.clear();
    collisionTree.objects.circle.clear();
    collisionTree.objects.triangle.clear();
    collisionTree.children.clear();
}

void DrawBrushStroke::allocate_collider() {
    create_collider(true);
}

SCollision::AABB<float> DrawBrushStroke::get_obj_coord_bounds() const {
    return collisionTree.objects.bounds;
}

#endif
