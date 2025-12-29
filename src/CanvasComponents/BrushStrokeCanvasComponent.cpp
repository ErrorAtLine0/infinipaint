#include "BrushStrokeCanvasComponent.hpp"
#include <Helpers/Serializers.hpp>
#include <cereal/types/vector.hpp>
#include <limits>
#include <Eigen/Geometry>
#include "Helpers/MathExtras.hpp"
#include "../TimePoint.hpp"
#include <include/core/SkVertices.h>
#include <include/pathops/SkPathOps.h>
#include "../DrawCollision.hpp"
#include "CanvasComponentContainer.hpp"

#define AABB_PRECHECK_BVH_LEVELS 2
#define DEFAULT_SMOOTHNESS 5

void BrushStrokeCanvasComponent::save(cereal::PortableBinaryOutputArchive& a) const {
    a(d->points, d->color, d->hasRoundCaps);
}

void BrushStrokeCanvasComponent::load(cereal::PortableBinaryInputArchive& a) {
    a(d->points, d->color, d->hasRoundCaps);
}

void BrushStrokeCanvasComponent::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(d->points, d->color, d->hasRoundCaps);
}

void BrushStrokeCanvasComponent::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    a(d->points, d->color, d->hasRoundCaps);
}

std::unique_ptr<CanvasComponent> BrushStrokeCanvasComponent::get_data_copy() const {
    auto toRet = std::make_unique<BrushStrokeCanvasComponent>();
    toRet->d = d;
    return toRet;
}

CanvasComponentType BrushStrokeCanvasComponent::get_type() const {
    return CanvasComponentType::BRUSHSTROKE;
}

void BrushStrokeCanvasComponent::draw(SkCanvas* canvas, const DrawData& drawData) const {
    SkPaint paint;
    paint.setColor4f(SkColor4f{d->color.x(), d->color.y(), d->color.z(), d->color.w()});

    unsigned mipmapLevel = compContainer ? compContainer->get_mipmap_level(drawData) : 0;

    if(mipmapLevel == 0 || drawData.dontUseDrawProgCache) // check dontUseDrawProgCache, to make sure that SVG screenshots dont use LOD
        canvas->drawPath(*brushPath, paint);
    else
        canvas->drawPath(*brushPathLOD[mipmapLevel - 1], paint);
}

void BrushStrokeCanvasComponent::set_data_from(const CanvasComponent& other) {
    auto& otherStroke = static_cast<const BrushStrokeCanvasComponent&>(other);
    d = otherStroke.d;
}

void BrushStrokeCanvasComponent::initialize_draw_data(DrawingProgram& drawP) {
    if(!d->points.empty()) {
        std::vector<BrushStrokeCanvasComponentPoint> points = smooth_points(0, d->points.size() - 1, DEFAULT_SMOOTHNESS);
        auto triangleFunc = [&](Vector2f a, Vector2f b, Vector2f c) {
            return false;
        };

        auto brushPathBuilder = std::make_shared<SkPathBuilder>();
        auto brushPathLOD0Builder = std::make_shared<SkPathBuilder>();
        auto brushPathLOD1Builder = std::make_shared<SkPathBuilder>();

        create_triangles(triangleFunc, points, 0, brushPathBuilder);
        create_triangles(triangleFunc, points, 4, brushPathLOD0Builder);
        create_triangles(triangleFunc, points, 30, brushPathLOD1Builder);

        brushPath = std::make_shared<SkPath>(brushPathBuilder->detach());
        brushPathLOD[0] = std::make_shared<SkPath>(brushPathLOD0Builder->detach());
        brushPathLOD[1] = std::make_shared<SkPath>(brushPathLOD1Builder->detach());
    }
    create_collider();
}

std::vector<size_t> BrushStrokeCanvasComponent::get_wedge_indices(const std::vector<BrushStrokeCanvasComponentPoint>& points) const {
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

std::vector<BrushStrokeCanvasComponentPoint> BrushStrokeCanvasComponent::smooth_points(size_t beginIndex, size_t endIndex, unsigned numOfDivisions) const {
    std::vector<BrushStrokeCanvasComponentPoint>& points = d->points;

    size_t pointsSize = endIndex - beginIndex + 1;
    if(pointsSize < 2) 
        return {points[beginIndex], points[endIndex]};

    std::vector<BrushStrokeCanvasComponentPoint> toRet;

    BrushStrokeCanvasComponentPoint p0 = points[beginIndex];
    p0.pos = p0.pos + p0.width * (p0.pos - points[beginIndex + 1].pos).normalized() * DRAW_MINIMUM_LIMIT;
    BrushStrokeCanvasComponentPoint p1 = points[beginIndex];
    BrushStrokeCanvasComponentPoint p2 = points[beginIndex + 1];
    BrushStrokeCanvasComponentPoint p3;

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
            BrushStrokeCanvasComponentPoint pToAdd;
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

std::vector<BrushStrokeCanvasComponentPoint> BrushStrokeCanvasComponent::smooth_points_avg(size_t beginIndex, size_t endIndex, unsigned numOfDivisions) const {
    std::vector<BrushStrokeCanvasComponentPoint>& points = d->points;

    size_t pointsSize = endIndex - beginIndex + 1;
    if(pointsSize < 2) 
        return {points[beginIndex], points[endIndex]};

    std::vector<BrushStrokeCanvasComponentPoint> toRet;

    for(size_t i = beginIndex; i < endIndex; i++) {
        const BrushStrokeCanvasComponentPoint& p1 = points[i];
        const BrushStrokeCanvasComponentPoint& p2 = points[i + 1];

        float tMove = 1.0 / static_cast<float>(numOfDivisions);
        for(unsigned d = 0; d < numOfDivisions; d++) {
            float t = tMove * static_cast<float>(d);
            BrushStrokeCanvasComponentPoint pToAdd;
            pToAdd.pos = lerp_vec(p1.pos, p2.pos, t);
            pToAdd.width = std::lerp(p1.width, p2.width, t);
            if(!(std::isnan(pToAdd.pos.x()) || std::isnan(pToAdd.pos.y()))) // Don't add NAN values
                toRet.emplace_back(pToAdd);
        }
    }

    toRet.emplace_back(points[endIndex]);

    return toRet;
}

std::vector<BrushStrokeCanvasComponentPoint> BrushStrokeCanvasComponent::every_nth_point_include_front_and_back(const std::vector<BrushStrokeCanvasComponentPoint>& pts, size_t n) const {
    if(pts.size() <= 3 || n == 0)
        return pts;
    std::vector<BrushStrokeCanvasComponentPoint> toRet;
    toRet.emplace_back(pts.front());
    for(size_t i = n; i < pts.size() - 1; i += n)
        toRet.emplace_back(pts[i]);
    toRet.emplace_back(pts.back());
    return toRet;
}

void BrushStrokeCanvasComponent::create_triangles(const std::function<bool(Vector2f, Vector2f, Vector2f)>& passTriangleFunc, const std::vector<BrushStrokeCanvasComponentPoint>& smoothedPoints, size_t skipVertexCount, std::shared_ptr<SkPathBuilder> bPathBuilder) const {
    const int ARC_SMOOTHNESS = 10;
    const int CIRCLE_SMOOTHNESS = 20;
    const std::vector<BrushStrokeCanvasComponentPoint>& pointsN = d->points;
    std::vector<SkPoint> topPoints;
    std::vector<SkPoint> bottomPoints;

    if(pointsN.size() == 1 && d->hasRoundCaps) {
        std::vector<Vector2f> circlePoints = gen_circle_points(pointsN.front().pos, pointsN.front().width / 2.0f, CIRCLE_SMOOTHNESS);
        for(size_t i = 0; i < circlePoints.size(); i++) {
            if(passTriangleFunc(pointsN.front().pos, circlePoints[i], circlePoints[(i + 1) % circlePoints.size()])) return;
            topPoints.emplace_back(convert_vec2<SkPoint>(circlePoints[i]));
        }
    }

    if(pointsN.size() < 2) {
        if(bPathBuilder && !topPoints.empty())
            bPathBuilder->addPolygon({topPoints.data(), topPoints.size()}, false);
        return;
    }

    auto points = every_nth_point_include_front_and_back(smoothedPoints, skipVertexCount);

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

        if(d->hasRoundCaps && pointsBegin == 0) {
            Vector2f arcDirStart = (convert_vec2<Vector2f>(vertArray[0]) - points[0].pos).normalized();
            Vector2f arcDirEnd = (convert_vec2<Vector2f>(vertArray[1]) - points[0].pos).normalized();
            Vector2f arcDirCenter = (points[0].pos - points[1].pos).normalized();
            std::vector<Vector2f> arcPoints = arc_vec<float>(points[0].pos, arcDirStart, arcDirEnd, arcDirCenter, points[0].width * 0.5f, ARC_SMOOTHNESS);
            arcPoints.emplace(arcPoints.begin(), convert_vec2<Vector2f>(vertArray[0]));
            arcPoints.emplace_back(convert_vec2<Vector2f>(vertArray[1]));
            for(size_t i = 0; i < arcPoints.size() - 1; i++) {
                if(passTriangleFunc(points[0].pos, arcPoints[i], arcPoints[i + 1])) return;
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
            if(passTriangleFunc(vertArray[0], vertArray[1], vertArray[2])) return;
            
            vertArray[newIndex] = points[j].pos - perp * 0.5f * points[j].width;
            bottomPoints.emplace_back(convert_vec2<SkPoint>(vertArray[newIndex]));
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
        topPoints.emplace_back(convert_vec2<SkPoint>(vertArray[newIndex]));
        newIndex = (newIndex + 1) % 3;
        if(passTriangleFunc(vertArray[0], vertArray[1], vertArray[2])) return;

        vertArray[newIndex] = points[pointsEnd].pos - perp * points[pointsEnd].width * 0.5;
        bottomPoints.emplace_back(convert_vec2<SkPoint>(vertArray[newIndex]));
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
                if(passTriangleFunc(wedgeCenter, arcPoints[i], arcPoints[i + 1])) return;
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
        else if(d->hasRoundCaps) { // It's the last point, cap it off
            Vector2f arcDirStart = (convert_vec2<Vector2f>(vertArray[(newIndex + 1) % 3]) - points.back().pos).normalized();
            Vector2f arcDirEnd = (convert_vec2<Vector2f>(vertArray[(newIndex + 2) % 3]) - points.back().pos).normalized();
            Vector2f arcDirCenter = -rectDir;
            std::vector<Vector2f> arcPoints = arc_vec<float>(points.back().pos, arcDirStart, arcDirEnd, arcDirCenter, points.back().width * 0.5f, ARC_SMOOTHNESS);
            arcPoints.emplace(arcPoints.begin(), convert_vec2<Vector2f>(vertArray[(newIndex + 1) % 3]));
            arcPoints.emplace_back(convert_vec2<Vector2f>(vertArray[(newIndex + 2) % 3]));
            for(size_t i = 0; i < arcPoints.size() - 1; i++) {
                if(passTriangleFunc(points.back().pos, arcPoints[i], arcPoints[i + 1])) return;
                topPoints.emplace_back(convert_vec2<SkPoint>(arcPoints[i]));
            }
            topPoints.emplace_back(convert_vec2<SkPoint>(arcPoints.back()));
        }
    }

    if(bPathBuilder && !topPoints.empty()) {
        topPoints.insert(topPoints.begin(), bottomPoints.rbegin(), bottomPoints.rend());
        bPathBuilder->addPolygon({topPoints.data(), topPoints.size()}, false);
    }
}

void BrushStrokeCanvasComponent::create_collider() {
    using namespace SCollision;
    if(d->points.empty())
        return;
    ColliderCollection<float> strokeObjects;
    std::vector<BrushStrokeCanvasComponentPoint> points = smooth_points(0, d->points.size() - 1, DEFAULT_SMOOTHNESS);
    create_triangles([&](Vector2f a, Vector2f b, Vector2f c) {
        strokeObjects.triangle.emplace_back(a, b, c);
        return false;
    }, points, 0, nullptr);
    strokeObjects.recalculate_bounds();

    SCollision::BVHContainer<float> collisionTree;

    collisionTree.calculate_bvh_recursive(strokeObjects, AABB_PRECHECK_BVH_LEVELS + 1);
    precheckAABBLevels.clear();
    add_precheck_aabb_level(AABB_PRECHECK_BVH_LEVELS, collisionTree.children);

    bounds = collisionTree.objects.bounds;
}

void BrushStrokeCanvasComponent::add_precheck_aabb_level(size_t level, const std::vector<SCollision::BVHContainer<float>>& levelArray) {
    for(const auto& a : levelArray) {
        if(level == 0 || a.children.empty())
            precheckAABBLevels.emplace_back(a.objects.bounds);
        else
            add_precheck_aabb_level(level - 1, a.children);
    }
}

bool BrushStrokeCanvasComponent::collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const {
    if(!SCollision::collide(checkAgainst, bounds))
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
    std::vector<BrushStrokeCanvasComponentPoint> points = smooth_points(0, d->points.size() - 1, DEFAULT_SMOOTHNESS);
    create_triangles([&](Vector2f a, Vector2f b, Vector2f c) {
        toRet = SCollision::collide(checkAgainst, SCollision::Triangle<float>{a, b, c});
        return toRet;
    }, points, 0, nullptr);
    return toRet;
}

SCollision::AABB<float> BrushStrokeCanvasComponent::get_obj_coord_bounds() const {
    return bounds;
}
