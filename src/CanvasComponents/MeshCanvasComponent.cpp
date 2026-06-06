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

#include "MeshCanvasComponent.hpp"
#include <Helpers/Serializers.hpp>
#include <cereal/types/vector.hpp>
#include <chrono>
#include <include/core/SkPathTypes.h>
#include <limits>
#include <Eigen/Geometry>
#include "BrushComponentCode.hpp"
#include "CanvasComponent.hpp"
#include "Eigen/Core"
#include "Helpers/MathExtras.hpp"
#include "../TimePoint.hpp"
#include <include/core/SkVertices.h>
#include <include/pathops/SkPathOps.h>
#include <memory>
#include "../DrawCollision.hpp"
#include "CanvasComponentContainer.hpp"
#include "Helpers/SCollision.hpp"
#include "../DrawingProgram/DrawingProgram.hpp"
#include "../World.hpp"
#include <CDT/include/CDT.h>
#include <clipper2/clipper.h>
#include <Helpers/Logger.hpp>

template <typename Archive> void skpath_write(const SkPath& p, Archive& a) {
    std::vector<std::vector<SkPoint>> contours;

    SkPath::Iter iter(p, true);
    for(;;) {
        std::optional<SkPath::IterRec> rec = iter.next();
        if(!rec.has_value())
            break;

        switch(rec->fVerb) {
            case SkPathVerb::kClose:
                if(!contours.back().empty())
                    contours.back().pop_back();
                break;
            case SkPathVerb::kLine:
                contours.back().emplace_back(rec->fPoints[1]);
                break;
            case SkPathVerb::kMove:
                contours.emplace_back();
                contours.back().emplace_back(rec->fPoints[0]);
                break;
            default:
                throw std::runtime_error("[get_predraw_data_accurate] Illegal verb " + std::to_string(static_cast<unsigned>(rec->fVerb)));
                break;
        }
    }

    a(p.getFillType() == SkPathFillType::kEvenOdd, contours);
}

template <typename Archive> SkPath skpath_read(Archive& a) {
    bool isEvenOdd;
    std::vector<std::vector<SkPoint>> contours;
    a(isEvenOdd, contours);
    // NOTE: setFillType doesn't properly set the fill type for the path IN DEBUG BUILDS. Setting fill type in builder constructor meanwhile works for both debug and release builds
    SkPathBuilder builder(isEvenOdd ? SkPathFillType::kEvenOdd : SkPathFillType::kWinding);
    for(const std::vector<SkPoint>& contour : contours) {
        if(contour.size() == 0)
            continue;
        builder.moveTo(contour[0]);
        for(uint32_t i = 1; i < contour.size(); i++)
            builder.lineTo(contour[i]);
        builder.close();
    }
    return builder.detach();
}

void MeshCanvasComponent::save(cereal::PortableBinaryOutputArchive& a) const {
    skpath_write(d.meshPath, a);
    a(d.color);
}

void MeshCanvasComponent::load(cereal::PortableBinaryInputArchive& a) {
    d.meshPath = skpath_read(a);
    a(d.color);
}

void MeshCanvasComponent::save_file(cereal::PortableBinaryOutputArchive& a) const {
    skpath_write(d.meshPath, a);
    a(d.color);
}

void MeshCanvasComponent::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    if(version >= VersionNumber(0, 6, 0)) {
        d.meshPath = skpath_read(a);
        a(d.color);
    }
    else {
        std::vector<BrushComponentCode::BrushPoint> brushPoints;
        bool hasRoundCaps;
        a(brushPoints, d.color, hasRoundCaps);
        d.meshPath = BrushComponentCode::brush_stroke_to_skpath(brushPoints, hasRoundCaps);
        std::optional<SkPath> simplified = Simplify(d.meshPath);
        if(simplified.has_value())
            d.meshPath = simplified.value();
    }
}

std::unique_ptr<CanvasComponent> MeshCanvasComponent::get_data_copy() const {
    auto toRet = std::make_unique<MeshCanvasComponent>();
    toRet->d = d;
    return toRet;
}

CanvasComponentType MeshCanvasComponent::get_type() const {
    return CanvasComponentType::MESH;
}

void MeshCanvasComponent::change_stroke_color(const Vector4f& newStrokeColor) {
    d.color = newStrokeColor;
}

std::optional<Vector4f> MeshCanvasComponent::get_stroke_color() const {
    return d.color;
}

void MeshCanvasComponent::draw(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<void>& predrawData) const {
    SkPaint paint;
    paint.setColor4f(SkColor4f{d.color.x(), d.color.y(), d.color.z(), d.color.w()});
    paint.setAntiAlias(drawData.skiaAA);
    canvas->drawPath(d.meshPath, paint);
}

std::shared_ptr<void> MeshCanvasComponent::get_predraw_data_accurate(const DrawData& drawData, const CoordSpaceHelper& coords) const {
    try {
        SCollision::AABB<float> viewGenerousColliderInObjSpace = coords.world_collider_to_coords<SCollision::AABB<float>>(drawData.cam.viewingAreaGenerousCollider);
        viewGenerousColliderInObjSpace.min -= Vector2f{1.0f, 1.0f};
        viewGenerousColliderInObjSpace.max += Vector2f{1.0f, 1.0f};

        // GrTriangulator is faster, but isn't as robust
        //GrCpuVertexAllocator cpuAlloc;
        //bool isLinear;
        //GrTriangulator::PathToTriangles(d.meshPath, 0.0001f, viewGenerousColliderInObjSpace.get_sk_rect(), &cpuAlloc, &isLinear);
        //sk_sp<GrThreadSafeCache::VertexData> detachedVertexData = cpuAlloc.detachVertexData();
        //if(!detachedVertexData)
        //    return nullptr;
        //if(detachedVertexData->vertexSize() != 8)
        //    throw std::runtime_error("[MeshCanvasComponent::get_predraw_data_accurate] Vertex data vertex size not equal to 8");

        // Triangulation implementation using CDT
        SkPath::Iter iter(d.meshPath, true);
        std::vector<CDT::V2d<float>> points;
        std::vector<CDT::Edge> edges;

        // Makes duplicate vertices by default, but duplicates are impossible to completely remove, so just rely on RemoveDuplicatesAndRemapEdges
        for(;;) {
            std::optional<SkPath::IterRec> rec = iter.next();
            if(!rec.has_value())
                break;

            switch(rec->fVerb) {
                case SkPathVerb::kClose:
                    break;
                case SkPathVerb::kLine: {
                    CDT::V2d<float> newPoint{rec->fPoints[1].x(), rec->fPoints[1].y()};
                    points.emplace_back(newPoint);
                    edges.emplace_back(points.size() - 2, points.size() - 1);
                    break;
                }
                case SkPathVerb::kMove:
                    points.emplace_back(rec->fPoints[0].x(), rec->fPoints[0].y());
                    break;
                default:
                    throw std::runtime_error("[get_predraw_data_accurate] Illegal verb " + std::to_string(static_cast<unsigned>(rec->fVerb)));
                    break;
            }
        }

        CDT::RemoveDuplicatesAndRemapEdges(
            points,
            edges
        );

        CDT::Triangulation<float> cdt(CDT::VertexInsertionOrder::Auto, CDT::IntersectingConstraintEdges::TryResolve, 0.001f);
        cdt.insertVertices(points);
        cdt.insertEdges(edges);
        cdt.eraseOuterTrianglesAndHoles();

        std::vector<std::array<SkPoint, 3>> finalTrianglePoints;

        auto clipListFunc = [](const std::vector<std::array<WorldVec, 3>>& clipList, const std::array<WorldVec, 2>& axisLineSegment, const std::function<bool(const WorldVec&)>& isInClippingAreaFunc) {
            std::vector<std::array<WorldVec, 3>> resultList;
            for(auto& t : clipList)
                clip_triangle_against_axis(resultList, t, axisLineSegment, isInClippingAreaFunc);
            return resultList;
        };

        // GrTriangulator
        //for(int i = 0; i < detachedVertexData->numVertices(); i += 3) {
        //    Vector2f v1 = {static_cast<const float*>(detachedVertexData->vertices())[i], static_cast<const float*>(detachedVertexData->vertices())[i + 1]};
        //    Vector2f v2 = {static_cast<const float*>(detachedVertexData->vertices())[i + 2], static_cast<const float*>(detachedVertexData->vertices())[i + 3]};
        //    Vector2f v3 = {static_cast<const float*>(detachedVertexData->vertices())[i + 4], static_cast<const float*>(detachedVertexData->vertices())[i + 5]};
        // CDT
        for(size_t i = 0; i < cdt.triangles.size(); i++) {
            auto& tri = cdt.triangles[i].vertices;
            Vector2f v1{cdt.vertices[tri[0]].x, cdt.vertices[tri[0]].y};
            Vector2f v2{cdt.vertices[tri[1]].x, cdt.vertices[tri[1]].y};
            Vector2f v3{cdt.vertices[tri[2]].x, cdt.vertices[tri[2]].y};
            SCollision::Triangle triCollider(v1, v2, v3);
            if(SCollision::collide(triCollider.bounds, viewGenerousColliderInObjSpace)) {
                std::vector<std::array<WorldVec, 3>> clipList;
                clipList.emplace_back(std::array<WorldVec, 3>{coords.from_space(triCollider.p[0]), coords.from_space(triCollider.p[1]), coords.from_space(triCollider.p[2])});
                clipList = clipListFunc(clipList, {drawData.cam.viewingAreaGenerousCollider.min, drawData.cam.viewingAreaGenerousCollider.top_right()}, [&](const WorldVec& p) {
                    return p.y() > drawData.cam.viewingAreaGenerousCollider.min.y();
                });
                clipList = clipListFunc(clipList, {drawData.cam.viewingAreaGenerousCollider.max, drawData.cam.viewingAreaGenerousCollider.bottom_left()}, [&](const WorldVec& p) {
                    return p.y() < drawData.cam.viewingAreaGenerousCollider.max.y();
                });
                clipList = clipListFunc(clipList, {drawData.cam.viewingAreaGenerousCollider.min, drawData.cam.viewingAreaGenerousCollider.bottom_left()}, [&](const WorldVec& p) {
                    return p.x() > drawData.cam.viewingAreaGenerousCollider.min.x();
                });
                clipList = clipListFunc(clipList, {drawData.cam.viewingAreaGenerousCollider.max, drawData.cam.viewingAreaGenerousCollider.top_right()}, [&](const WorldVec& p) {
                    return p.x() < drawData.cam.viewingAreaGenerousCollider.max.x();
                });
                for(auto& tri : clipList) {
                    auto& c = drawData.cam.c;
                    finalTrianglePoints.emplace_back(std::array<SkPoint, 3>{convert_vec2<SkPoint>(c.to_space(tri[0])), convert_vec2<SkPoint>(c.to_space(tri[1])), convert_vec2<SkPoint>(c.to_space(tri[2]))});
                }
            }
        }

        // Match points that are close enough. I don't think this is required.
        //if(!finalTrianglePoints.empty()) {
        //    for(size_t i = 0; i < finalTrianglePoints.size() - 1; i++) {
        //        for(size_t j = 0; j < 3; j++) {
        //            for(size_t k = i + 1; k < finalTrianglePoints.size(); k++) {
        //                for(size_t l = 0; l < 3; l++) {
        //                    if(std::fabs(finalTrianglePoints[i][j].fX - finalTrianglePoints[k][l].fX) < 0.0001f && std::fabs(finalTrianglePoints[i][j].fY - finalTrianglePoints[k][l].fY) < 0.0001f && finalTrianglePoints[k][l] != finalTrianglePoints[i][j])
        //                        finalTrianglePoints[k][l] = finalTrianglePoints[i][j];
        //                }
        //            }
        //        }
        //    }
        //}

        if(finalTrianglePoints.empty())
            return nullptr;
        SkPathBuilder pathBuilder;
        for(auto& finalTriangle : finalTrianglePoints)
            pathBuilder.addPolygon({finalTriangle.data(), finalTriangle.size()}, true);
        return std::make_shared<SkPath>(pathBuilder.detach());
    }
    catch(...) {
        return nullptr;
    }
    return nullptr;
}

bool MeshCanvasComponent::accurate_draw(SkCanvas* canvas, const DrawData& drawData, const CoordSpaceHelper& coords, const std::shared_ptr<void>& predrawData) const {
    if(predrawData) {
        auto pathToDraw = std::static_pointer_cast<SkPath>(predrawData);
        if(pathToDraw) {
            SkPaint paint;
            paint.setColor4f(SkColor4f{d.color.x(), d.color.y(), d.color.z(), d.color.w()});
            paint.setAntiAlias(drawData.skiaAA);
            canvas->drawPath(*pathToDraw, paint);
        }
    }

    return true;
}

void MeshCanvasComponent::set_data_from(const CanvasComponent& other) {
    auto& otherStroke = static_cast<const MeshCanvasComponent&>(other);
    d = otherStroke.d;
}

void MeshCanvasComponent::initialize_draw_data(DrawingProgram& drawP) {
}

bool MeshCanvasComponent::collides_within_coords_point(const Vector2f& checkAgainst) const {
    bool intersectsAABB = d.meshPath.getBounds().contains(checkAgainst.x(), checkAgainst.y());
    if(!intersectsAABB)
        return false;
    return d.meshPath.contains(checkAgainst.x(), checkAgainst.y());
}

bool MeshCanvasComponent::collides_within_coords_skpath(const SkPath& checkAgainst) const {
    bool intersectsAABB = d.meshPath.getBounds().intersects(checkAgainst.getBounds());
    if(!intersectsAABB)
        return false;
    std::optional<SkPath> pathIntersectCheck = Op(checkAgainst, d.meshPath, SkPathOp::kIntersect_SkPathOp);
    return pathIntersectCheck.has_value() && !pathIntersectCheck.value().isEmpty();
}

bool MeshCanvasComponent::should_draw_extra(const DrawData& drawData, const CoordSpaceHelper& coords) const {
    return true;
}

bool MeshCanvasComponent::can_erase_detail() const {
    return true;
}

std::vector<CanvasComponentContainer*> MeshCanvasComponent::attempt_split(DrawingProgram& drawP) const {
    using namespace Clipper2Lib;

    std::vector<CanvasComponentContainer*> toRet;

    PathsD clippingSubjects;

    BrushComponentCode::skpath_to_clipper2_pathsd(clippingSubjects, d.meshPath);

    if(clippingSubjects.size() <= 1)
        return {};

    ClipperD clipper(4);
    PolyTreeD solutionTree;

    clipper.AddSubject(clippingSubjects);
    clipper.AddClip(clippingSubjects);
    clipper.Execute(ClipType::Union, d.meshPath.getFillType() == SkPathFillType::kEvenOdd ? FillRule::EvenOdd : FillRule::NonZero, solutionTree);

    std::vector<SkPath> splitPaths;

    BrushComponentCode::sort_clipper_polytreed_into_skpaths(splitPaths, solutionTree);

    if(splitPaths.size() <= 1)
        return toRet;

    for(const SkPath& p : splitPaths) {
        CanvasComponentContainer* newComp = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::MESH);
        MeshCanvasComponent& mesh = static_cast<MeshCanvasComponent&>(newComp->get_comp());
        mesh.d.color = d.color;
        mesh.d.meshPath = p;
        newComp->coords = compContainer->coords;
        mesh.simplify_paths();
        newComp->normalize_object_coordinates();
        toRet.emplace_back(newComp);
    }

    return toRet;
}

void MeshCanvasComponent::normalize_object_coordinates(CoordSpaceHelper& coords) {
    SkRect pathBounds = d.meshPath.getBounds();
    SkPoint center = pathBounds.center();
    constexpr float MAX_COORDINATE = 1000.0f;
    float maxDimScaleFactor = MAX_COORDINATE / (std::max(pathBounds.width(), pathBounds.height()) * 0.5f); // Scale must be uniform. This scale factor should force the maximum coordinate = MAX_COORDINATE after transformation
    SkMatrix m = SkMatrix::I();
    m.postTranslate(-center.x(), -center.y()).postScale(maxDimScaleFactor, maxDimScaleFactor);
    std::optional<SkPath> tryTransformPath = d.meshPath.tryMakeTransform(m);
    if(tryTransformPath) {
        d.meshPath = tryTransformPath.value();
        WorldVec newCenter = WorldVec{center.x(), center.y()} * coords.inverseScale;
        newCenter = rotate_world_coord(newCenter, coords.rotation);
        coords.translate(newCenter);
        coords.scale_about_double(coords.pos, maxDimScaleFactor);
    }
}

CanvasComponentEraseDetailResult MeshCanvasComponent::erase_detail(const SkPath& eraseAgainst) {
    bool intersectsAABB = d.meshPath.getBounds().intersects(eraseAgainst.getBounds());
    if(!intersectsAABB)
        return CanvasComponentEraseDetailResult::NO_CHANGE;
    std::optional<SkPath> newPath = Op(Simplify(d.meshPath).value(), Simplify(eraseAgainst).value(), SkPathOp::kDifference_SkPathOp);
    if(newPath.has_value()) {
        if(newPath.value().isEmpty()) {
            d.meshPath = newPath.value();
            return CanvasComponentEraseDetailResult::REMOVED;
        }
        else if(!d.meshPath.getBounds().contains(newPath.value().getBounds())) // If bounds of erased path is larger than bounds of original path (can happen when difference op bugs out with big eraser paths), dont change the path
            return CanvasComponentEraseDetailResult::NO_CHANGE;
        else {
            d.meshPath = newPath.value();
            return CanvasComponentEraseDetailResult::CHANGED;
        }
    }
    return CanvasComponentEraseDetailResult::NO_CHANGE;
}

void MeshCanvasComponent::simplify_paths() {
    auto newPath = Simplify(d.meshPath);
    if(newPath.has_value())
        d.meshPath = newPath.value();
}

SCollision::AABB<float> MeshCanvasComponent::get_obj_coord_bounds() const {
    return d.meshPath.getBounds();
}
