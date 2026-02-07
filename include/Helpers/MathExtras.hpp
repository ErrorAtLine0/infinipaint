#pragma once
#include <cmath>
#include <Eigen/Dense>
#include <array>
#include <limits>
#include <optional>
#include <utility>
#include <iostream>
#include <numbers>

using namespace Eigen;

template <typename T> T color_mul_alpha(const T& c, float a) {
    return T{c[0], c[1], c[2], c[3] * a};
}

template <typename T> T circular_mod(T a, T b) {
    if(a >= 0)
        return a % b;
    else
        return b - (std::abs(a) % b);
}

template <typename T> T circular_fmod(T a, T b) {
    if(a >= 0)
        return std::fmod(a, b);
    else
        return b - std::fmod(a, b);
}

template <typename T, typename MultipleType> T round_to_multiple(T a, MultipleType multiple) {
    T toRet = round(a / multiple) * multiple;
    if((toRet == +0.0 || toRet == -0.0) && std::signbit(toRet))
        toRet = 0.0;
    return toRet;
}

template <typename T, typename MultipleType> T round_vec_to_multiple(T a, MultipleType multiple) {
    for(size_t i = 0; i < static_cast<size_t>(a.size()); i++)
        a[i] = round_to_multiple(a[i], multiple);
    return a;
}

template <typename T> Matrix<T, 2, 1> perpendicular_vec2(const Matrix<T, 2, 1>& vec) {
    return Matrix<T, 2, 1>{-vec.y(), vec.x()};
}

template <typename T> T dist_point_line_segment(const Matrix<T, 2, 1>& point, const Matrix<T, 2, 1>& lineP1, const Matrix<T, 2, 1>& lineP2) {
    T lenSqrd = (lineP1 - lineP2).squaredNorm();
    if(lenSqrd == 0)
        return (point - lineP1).norm();
    T t = std::max<T>(0, std::min<T>(1, (point - lineP1).dot(lineP2 - lineP1) / lenSqrd));
    return (point - (lineP1 + t * (lineP2 - lineP1))).norm();
}

template <typename T> bool collision_circle_line_segment(const Matrix<T, 2, 1>& circleCenter, T circleRadius, const Matrix<T, 2, 1>& lineP1, const Matrix<T, 2, 1>& lineP2) {
    return dist_point_line_segment(circleCenter, lineP1, lineP2) <= circleRadius;
}

// https://stackoverflow.com/a/53896859
template <typename T> bool is_collision_ray_line_segment(const Vector<T, 2>& rayOrigin, const Vector<T, 2>& rayDirection, const Vector<T, 2>& point1, const Vector<T, 2>& point2) {
    Vector<T, 2> v1 = rayOrigin - point1;
    Vector<T, 2> v2 = point2 - point1;
    Vector<T, 2> v3 = Vector<T, 2>{-rayDirection.y(), rayDirection.x()};

    Vector<T, 3> v2three{v2.x(), v2.y(), 0.0};
    Vector<T, 3> v1three{v1.x(), v1.y(), 0.0};

    T dot = v2.dot(v3);
    if (std::abs(dot) < 0.000001)
        return false;

    T t1 = v2three.cross(v1three) / dot;
    T t2 = v1.dot(v3) / dot;

    return (t1 >= 0.0 && (t2 >= 0.0 && t2 <= 1.0));
}

// https://www.geeksforgeeks.org/program-for-point-of-intersection-of-two-lines/
template <typename T> Vector<T, 2> line_line_intersection(const Vector<T, 2>& a, const Vector<T, 2>& b, const Vector<T, 2>& c, const Vector<T, 2>& d) {
    T a1 = b.y() - a.y();
    T b1 = a.x() - b.x();
    T c1 = a1*(a.x()) + b1*(a.y());
 
    T a2 = d.y() - c.y();
    T b2 = c.x() - d.x();
    T c2 = a2*(c.x())+ b2*(c.y());
 
    T determinant = a1*b2 - a2*b1;
 
    T x = (b2*c1 - b1*c2)/determinant;
    T y = (a1*c2 - a2*c1)/determinant;

    if(std::fabs(determinant) < 0.0000001f)
        return Vector<T, 2>{std::numeric_limits<T>::max(), std::numeric_limits<T>::max()};

    return Vector<T, 2>{x, y};
}

template <typename T> Vector<T, 2> project_point_on_vec(const Vector<T, 2>& point, const Vector<T, 2> unnormalizedVec) {
    return ((point.dot(unnormalizedVec)) / unnormalizedVec.dot(unnormalizedVec)) * unnormalizedVec;
}

template <typename T> Vector<T, 2> project_point_on_normalized_vec(const Vector<T, 2>& point, const Vector<T, 2> normalizedVec) {
    return point.dot(normalizedVec) * normalizedVec;
}

// https://stackoverflow.com/a/61342198
template <typename T> Vector<T, 2> project_point_on_line(const Vector<T, 2>& p, const Vector<T, 2>& b, const Vector<T, 2>& a) {
    Vector<T, 2> ap = p - a;
    Vector<T, 2> ab = b - a;

    T t = ap.dot(ab) / ab.dot(ab);

    return a + t * ab;
}

// https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
template <typename T> Vector<T, 2> line_line_intersection_inaccurate(const Matrix<T, 2, 1>& a, const Matrix<T, 2, 1>& b, const Matrix<T, 2, 1>& c, const Matrix<T, 2, 1>& d) {
    T x1 = a.x(); T x2 = b.x(); T x3 = c.x(); T x4 = d.x();
    T y1 = a.y(); T y2 = b.y(); T y3 = c.y(); T y4 = d.y();
    T p1 = ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / ((x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4));
    T p2 = ((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) / ((x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4));
    return Vector<T, 2>{p1, p2};
}

// https://www.reddit.com/r/algorithms/comments/9moad4/what_is_the_simplest_to_implement_line_segment/
// Taken from https://github.com/vlecomte/cp-geo
template <typename T> T cross_vals_vec(const Matrix<T, 2, 1>& a, const Matrix<T, 2, 1>& b) {
    return a.x() * b.y() - a.y() * b.x();
}

template <typename T> T orient_vals_vec(const Matrix<T, 2, 1>& a, const Matrix<T, 2, 1>& b, const Matrix<T, 2, 1>& c) {
    return cross_vals_vec((b - a).eval(), (c - a).eval());
}

template <typename T> bool is_collision_line_segment_line_segment(const Matrix<T, 2, 1>& a, const Matrix<T, 2, 1>& b, const Matrix<T, 2, 1>& c, const Matrix<T, 2, 1>& d) {
    T oa = orient_vals_vec(c, d, a);
    T ob = orient_vals_vec(c, d, b);
    T oc = orient_vals_vec(a, b, c);
    T od = orient_vals_vec(a, b, d);
    return (oa * ob < T(0) && oc * od < T(0));
}

template <typename T> std::optional<Matrix<T, 2, 1>> collision_line_segment_line_segment_pos(const Matrix<T, 2, 1>& a, const Matrix<T, 2, 1>& b, const Matrix<T, 2, 1>& c, const Matrix<T, 2, 1>& d) {
    T oa = orient_vals_vec(c, d, a);
    T ob = orient_vals_vec(c, d, b);
    T oc = orient_vals_vec(a, b, c);
    T od = orient_vals_vec(a, b, d);
    if(oa * ob < T(0) && oc * od < T(0))
        return std::optional<Matrix<T, 2, 1>>((a * ob - b * oa) / (ob - oa));
    return std::nullopt;
}

template <typename T> Matrix<T, 2, 1> collision_line_segment_line_segment_guaranteed(const Matrix<T, 2, 1>& a, const Matrix<T, 2, 1>& b, const Matrix<T, 2, 1>& c, const Matrix<T, 2, 1>& d) {
    T oa = orient_vals_vec(c, d, a);
    T ob = orient_vals_vec(c, d, b);
    T oc = orient_vals_vec(a, b, c);
    T od = orient_vals_vec(a, b, d);
    return Matrix<T, 2, 1>((a * ob - b * oa) / (ob - oa));
}

template <typename T> bool is_collision_aabb_line_segment(const AlignedBox<T, 2>& aabb, const Matrix<T, 2, 1>& lineP1, const Matrix<T, 2, 1>& lineP2) {
    return aabb.contains(lineP1) ||
           aabb.contains(lineP2) ||
           is_collision_line_segment_line_segment(lineP1, lineP2, aabb.corner(AlignedBox<T, 2>::TopLeft),    aabb.corner(AlignedBox<T, 2>::TopRight))    ||
           is_collision_line_segment_line_segment(lineP1, lineP2, aabb.corner(AlignedBox<T, 2>::TopRight),   aabb.corner(AlignedBox<T, 2>::BottomRight)) ||
           is_collision_line_segment_line_segment(lineP1, lineP2, aabb.corner(AlignedBox<T, 2>::BottomLeft), aabb.corner(AlignedBox<T, 2>::BottomRight)) ||
           is_collision_line_segment_line_segment(lineP1, lineP2, aabb.corner(AlignedBox<T, 2>::TopLeft),    aabb.corner(AlignedBox<T, 2>::BottomLeft));
}

template <typename T> std::vector<Matrix<T, 2, 1>> collision_aabb_small_line_segment_large(const AlignedBox<T, 2>& aabb, const Matrix<T, 2, 1>& lineP1, const Matrix<T, 2, 1>& lineP2) {
    std::vector<Matrix<T, 2, 1>> toRet;
    auto optCollision = collision_line_segment_line_segment_pos(lineP1, lineP2, aabb.corner(AlignedBox<T, 2>::TopLeft), aabb.corner(AlignedBox<T, 2>::TopRight));
    if(optCollision.has_value())
        toRet.emplace_back(optCollision.value());
    optCollision = collision_line_segment_line_segment_pos(lineP1, lineP2, aabb.corner(AlignedBox<T, 2>::TopRight), aabb.corner(AlignedBox<T, 2>::BottomRight));
    if(optCollision.has_value())
        toRet.emplace_back(optCollision.value());
    optCollision = collision_line_segment_line_segment_pos(lineP1, lineP2, aabb.corner(AlignedBox<T, 2>::BottomLeft), aabb.corner(AlignedBox<T, 2>::BottomRight));
    if(optCollision.has_value())
        toRet.emplace_back(optCollision.value());
    optCollision = collision_line_segment_line_segment_pos(lineP1, lineP2, aabb.corner(AlignedBox<T, 2>::TopLeft), aabb.corner(AlignedBox<T, 2>::BottomLeft));
    if(optCollision.has_value())
        toRet.emplace_back(optCollision.value());
    return toRet;
}

template <typename T> T vec_distance(const Vector<T, 2>& v1, const Vector<T, 2>& v2) {
    return (v1 - v2).norm();
}

template <typename T> T vec_distance_sqrd(const Vector<T, 2>& v1, const Vector<T, 2>& v2) {
    return (v1 - v2).squaredNorm();
}

template <typename T> T vec_length(const Vector<T, 2>& v) {
    return v.norm();
}

template <typename T> T cwise_vec_min(const T& v1, const T& v2) {
    T toRet;
    for(unsigned i = 0; i < v1.size(); i++)
        toRet[i] = std::min(v1[i], v2[i]);
    return toRet;
}

template <typename T> T cwise_vec_max(const T& v1, const T& v2) {
    T toRet;
    for(unsigned i = 0; i < v1.size(); i++)
        toRet[i] = std::max(v1[i], v2[i]);
    return toRet;
}

template <typename T> T cwise_vec_clamp(const T& v1, const T& v2, const T& v3) {
    T toRet;
    for(unsigned i = 0; i < v1.size(); i++)
        toRet[i] = std::clamp(v1[i], v2[i], v3[i]);
    return toRet;
}

template <typename T> T cwise_vec_modf(const T& v1, T& v2) {
    T toRet;
    for(unsigned i = 0; i < v1.size(); i++)
        toRet[i] = std::modf(v1[i], &v2[i]);
    return toRet;
}


template <typename T> std::vector<Vector<T, 2>> arc_vec(const Vector<T, 2>& arcCenter, const Vector<T, 2>& arcDirStart, const Vector<T, 2>& arcDirEnd, const Vector<T, 2>& arcDirCenter, T arcRadius, unsigned numOfDivisions) {
    T a1 = std::atan2(arcDirStart.y(), arcDirStart.x());
    T a2 = std::atan2(arcDirEnd.y(), arcDirEnd.x());
    T a3 = std::atan2(arcDirCenter.y(), arcDirCenter.x());

    T minAngle = std::min(a1, a2);
    T maxAngle = std::max(a1, a2);

    bool swapHappen = (a3 > minAngle && a3 > maxAngle) || (a3 < minAngle && a3 < maxAngle);
    if(swapHappen) {
        minAngle += 2.0 * std::numbers::pi;
        std::swap(minAngle, maxAngle);
    }

    bool flipped = minAngle == a2;

    std::vector<Vector<T, 2>> toRet;
    T tStep = 1.0 / static_cast<T>(numOfDivisions);
    if(flipped) {
        for(unsigned i = numOfDivisions - 1; i > 0; i--) {
            T newAngle = std::lerp(minAngle, maxAngle, tStep * i);
            toRet.emplace_back(arcCenter + Vector<T, 2>{std::cos(newAngle), std::sin(newAngle)} * arcRadius);
        }
    }
    else {
        for(unsigned i = 1; i < numOfDivisions; i++) {
            T newAngle = std::lerp(minAngle, maxAngle, tStep * i);
            toRet.emplace_back(arcCenter + Vector<T, 2>{std::cos(newAngle), std::sin(newAngle)} * arcRadius);
        }
    }
    return toRet;
}

template <typename T> std::vector<Vector<T, 2>> gen_circle_points(const Vector<T, 2>& center, T radius, unsigned numOfPoints) {
    std::vector<Vector<T, 2>> toRet;
    for(unsigned i = 0; i < numOfPoints; i++) {
        T angle = (static_cast<T>(i) / static_cast<T>(numOfPoints)) * std::numbers::pi * 2;
        toRet.emplace_back(std::cos(angle) * radius + center.x(), std::sin(angle) * radius + center.y());
    }
    return toRet;
}

// https://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline
template <typename T> T catmull_rom_get_t(T t, T alpha, const Vector<T, 2>& p0, const Vector<T, 2>& p1) {
    Vector<T, 2> d = p1 - p0;
    T a = d.dot(d);
    T b = std::pow(a, alpha * 0.5);
    return (b + t);
}
template <typename T> Vector<T, 2> catmull_rom(const Vector<T, 2>& p0, const Vector<T, 2>& p1, const Vector<T, 2>& p2, const Vector<T, 2>& p3, T t, T alpha = 0.5) {
    T t0 = 0.0;
    T t1 = catmull_rom_get_t(t0, alpha, p0, p1);
    T t2 = catmull_rom_get_t(t1, alpha, p1, p2);
    T t3 = catmull_rom_get_t(t2, alpha, p2, p3);
    t = std::lerp(t1, t2, t);
    Vector<T, 2> A1 = ( t1-t )/( t1-t0 )*p0 + ( t-t0 )/( t1-t0 )*p1;
    Vector<T, 2> A2 = ( t2-t )/( t2-t1 )*p1 + ( t-t1 )/( t2-t1 )*p2;
    Vector<T, 2> A3 = ( t3-t )/( t3-t2 )*p2 + ( t-t2 )/( t3-t2 )*p3;
    Vector<T, 2> B1 = ( t2-t )/( t2-t0 )*A1 + ( t-t0 )/( t2-t0 )*A2;
    Vector<T, 2> B2 = ( t3-t )/( t3-t1 )*A2 + ( t-t1 )/( t3-t1 )*A3;
    Vector<T, 2> C  = ( t2-t )/( t2-t1 )*B1 + ( t-t1 )/( t2-t1 )*B2;
    return C;
}

template <typename T, int S> T transform_scalar(const Transform<T, 2, S>& transform, T scalar) {
    return (transform.linear() * Vector<T, 2>{scalar, scalar}).x();
}

template <typename T> std::array<Vector<T, 2>, 4> triangle_from_rect_points(const Vector<T, 2>& min, const Vector<T, 2>& max) {
    return {min, {min.x(), max.y()}, max, {max.x(), min.y()}};
}

template <typename T> std::string vec_pretty(const Vector<T, 2>& a) {
    std::stringstream ss;
    ss << "[" << a.x() << ", " << a.y() << "]";
    return ss.str();
}

template <typename T> Vector<T, 2> ensure_points_have_distance(const Vector<T, 2>& a, const Vector<T, 2>& b, T d) {
    Vector<T, 2> toRet = b;
    if(std::abs(a.x() - b.x()) < d)
        toRet.x() = a.x() + d;
    if(std::abs(a.y() - b.y()) < d)
        toRet.y() = a.y() + d;
    return toRet;
}

// https://www.gabrielgambetta.com/computer-graphics-from-scratch/11-clipping.html
template <typename T> void clip_triangle_against_axis(std::vector<std::array<Vector<T, 2>, 3>>& triangleList, const std::array<Vector<T, 2>, 3>& t, const std::array<Vector<T, 2>, 2>& axisLineSegment, const std::function<bool(const Vector<T, 2>&)>& isInClippingAreaFunc) {
    std::array<bool, 3> inClippingArea = {
        isInClippingAreaFunc(t[0]),
        isInClippingAreaFunc(t[1]),
        isInClippingAreaFunc(t[2])
    };

    auto oneVertexInClippingArea = [&](const Vector<T, 2>& a, const Vector<T, 2>& b, const Vector<T, 2>& c) {
        Vector<T, 2> bPrime = collision_line_segment_line_segment_guaranteed(a, b, axisLineSegment[0], axisLineSegment[1]);
        Vector<T, 2> cPrime = collision_line_segment_line_segment_guaranteed(a, c, axisLineSegment[0], axisLineSegment[1]);
        triangleList.emplace_back(std::array<Vector<T, 2>, 3>{a, bPrime, cPrime});
    };

    auto twoVerticesInClippingArea = [&](const Vector<T, 2>& a, const Vector<T, 2>& b, const Vector<T, 2>& c) {
        Vector<T, 2> aPrime = collision_line_segment_line_segment_guaranteed(a, c, axisLineSegment[0], axisLineSegment[1]);
        Vector<T, 2> bPrime = collision_line_segment_line_segment_guaranteed(b, c, axisLineSegment[0], axisLineSegment[1]);
        triangleList.emplace_back(std::array<Vector<T, 2>, 3>{a, b, aPrime});
        triangleList.emplace_back(std::array<Vector<T, 2>, 3>{aPrime, b, bPrime});
    };

    if(inClippingArea[0] && inClippingArea[1] && inClippingArea[2])
        triangleList.emplace_back(t);
    else if(!inClippingArea[0] && !inClippingArea[1] && !inClippingArea[2])
        return;
    else if(inClippingArea[0] && !inClippingArea[1] && !inClippingArea[2])
        oneVertexInClippingArea(t[0], t[1], t[2]);
    else if(!inClippingArea[0] && inClippingArea[1] && !inClippingArea[2])
        oneVertexInClippingArea(t[1], t[0], t[2]);
    else if(!inClippingArea[0] && !inClippingArea[1] && inClippingArea[2])
        oneVertexInClippingArea(t[2], t[0], t[1]);
    else if(inClippingArea[0] && inClippingArea[1] && !inClippingArea[2])
        twoVerticesInClippingArea(t[0], t[1], t[2]);
    else if(inClippingArea[0] && !inClippingArea[1] && inClippingArea[2])
        twoVerticesInClippingArea(t[0], t[2], t[1]);
    else if(!inClippingArea[0] && inClippingArea[1] && inClippingArea[2])
        twoVerticesInClippingArea(t[1], t[2], t[0]);
}
