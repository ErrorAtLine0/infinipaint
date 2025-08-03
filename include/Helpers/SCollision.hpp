#pragma once
#include "Helpers/MathExtras.hpp"
#include <Eigen/Dense>
#include <array>
#include <limits>
#include <iostream>
#include <ostream>
#include <Eigen/Geometry>

using namespace Eigen;

namespace SCollision {
    template <typename T> class AABB {
        public:
            typedef T UnderlyingType;

            AABB() = default; // NOTE: Should not use the min and max numeric limits to initialize, since that wont work for WorldScalar
            AABB(const Vector<T, 2>& initMin, const Vector<T, 2>& initMax):
                min(initMin),
                max(initMax)
            {}
            Vector<T, 2> min;
            Vector<T, 2> max;
            Vector<T, 2> center() const { return (min + max) / T(2); }
            Vector<T, 2> bottom_left() const { return Vector<T, 2>{min.x(), max.y()}; }
            Vector<T, 2> top_right() const { return Vector<T, 2>{max.x(), min.y()}; }
            Vector<T, 2> dim() const { return max - min; }
            T width() const { return max.x() - min.x(); }
            T height() const { return max.y() - min.y(); }
            // Do not try to get intersection between AABBs unless you know they collide
            AABB<T> get_intersection_between_aabbs(const AABB<T>& other) {
                return {
                    cwise_vec_max(other.min, min),
                    cwise_vec_min(other.max, max)
                };
            }
            void include_aabb_in_bounds(const AABB<T>& other) {
                min = cwise_vec_min(other.min, min);
                max = cwise_vec_max(other.max, max);
            }
            void include_point_in_bounds(const Vector<T, 2>& p) {
                min = cwise_vec_min(p, min);
                max = cwise_vec_max(p, max);
            }
            bool operator==(const AABB<T>& other) const {
                return min == other.min && max == other.max;
            }
            bool fully_contains_aabb(const AABB<T>& other) const {
                return other.min.x() >= min.x() && other.min.y() >= min.y() && other.max.x() <= max.x() && other.max.y() <= max.y();
            }
            template <typename S> AABB<S> transform(const std::function<Vector<S, 2>(const Vector<T, 2>&)>& transformFunc, const std::function<S(const T&)>& transformLinearFunc) const {
                Vector<S, 2> aabb1 = transformFunc(min);
                Vector<S, 2> aabb2 = transformFunc(max);
                Vector<S, 2> aabb3 = transformFunc(top_right());
                Vector<S, 2> aabb4 = transformFunc(bottom_left());

                AABB<S> toRet{aabb1, aabb1};

                toRet.include_point_in_bounds(aabb1);
                toRet.include_point_in_bounds(aabb2);
                toRet.include_point_in_bounds(aabb3);
                toRet.include_point_in_bounds(aabb4);

                return toRet;
            }
            template <typename S> AABB<S> cast() {
                AABB<S> toRet{min.template cast<S>(), max.template cast<S>()};
                return toRet;
            }
    };
    template <typename T> class Circle {
        public:
            typedef T UnderlyingType;

            Circle() = default;
            Circle(const Vector<T, 2>& initPos, T initRadius):
                pos(initPos),
                radius(initRadius),
                bounds(pos - Vector<T, 2>{radius, radius}, pos + Vector<T, 2>{radius, radius})
            {}
            Vector<T, 2> pos;
            T radius;
            AABB<T> bounds;
            template <typename S> Circle<S> transform(const std::function<Vector<S, 2>(const Vector<T, 2>&)>& transformFunc, const std::function<S(const T&)>& transformLinearFunc) const {
                Circle<S> newCirc = Circle<S>();
                newCirc.pos = transformFunc(pos);
                newCirc.radius = transformLinearFunc(radius);
                newCirc.bounds = AABB<S>(newCirc.pos - Vector<S, 2>{newCirc.radius, newCirc.radius}, newCirc.pos + Vector<S, 2>{newCirc.radius, newCirc.radius});
                return newCirc;
            }
            template <typename S> Circle<S> cast() {
                Circle<S> toRet;
                toRet.pos = pos.template cast<S>();
                toRet.radius = static_cast<S>(radius);
                toRet.bounds = bounds.template cast<S>();
                return toRet;
            }
    };
    template <typename T> class Triangle {
        public:
            typedef T UnderlyingType;

            Triangle() = default;
            Triangle(const std::array<Vector<T, 2>, 3>& initP):
                p(initP),
                bounds(cwise_vec_min(cwise_vec_min(p[0], p[1]), p[2]), cwise_vec_max(cwise_vec_max(p[0], p[1]), p[2]))
            {}
            Triangle(const Vector<T, 2>& p1, const Vector<T, 2>& p2, const Vector<T, 2>& p3):
                p({p1, p2, p3}),
                bounds(cwise_vec_min(cwise_vec_min(p[0], p[1]), p[2]), cwise_vec_max(cwise_vec_max(p[0], p[1]), p[2]))
            {}
            template <typename S> Triangle<S> transform(const std::function<Vector<S, 2>(const Vector<T, 2>&)>& transformFunc, const std::function<S(const T&)>& transformLinearFunc) const {
                Triangle<S> toRet = Triangle<S>();
                toRet.p[0] = transformFunc(p[0]);
                toRet.p[1] = transformFunc(p[1]);
                toRet.p[2] = transformFunc(p[2]);
                toRet.bounds = AABB<S>(cwise_vec_min(cwise_vec_min(toRet.p[0], toRet.p[1]), toRet.p[2]), cwise_vec_max(cwise_vec_max(toRet.p[0], toRet.p[1]), toRet.p[2]));
                return toRet;
            }
            template <typename S> Triangle<S> cast() {
                Triangle<S> toRet;
                toRet.p[0] = p[0].template cast<S>();
                toRet.p[1] = p[1].template cast<S>();
                toRet.p[2] = p[2].template cast<S>();
                toRet.bounds = bounds.template cast<S>();
                return toRet;
            }
            std::array<Vector<T, 2>, 3> p;
            AABB<T> bounds;
    };

    template <typename T> struct ColliderCollection {
        typedef T UnderlyingType;

        AABB<T> bounds;
        std::vector<AABB<T>> aabb;
        std::vector<Circle<T>> circle;
        std::vector<Triangle<T>> triangle;

        void clear() {
            aabb.clear();
            circle.clear();
            triangle.clear();
            bounds = AABB<T>({0, 0}, {0, 0});
        }
        size_t size() const { return aabb.size() + circle.size() + triangle.size(); }
        bool empty() const { return aabb.empty() && circle.empty() && triangle.empty(); }
        template <typename ColliderList> void bound_limit(const ColliderList& list) {
            for(auto& o : list)
                bounds.include_aabb_in_bounds(get_bounds(o));
        }
        void recalculate_bounds() {
            bounds = AABB<T>();
            if(!aabb.empty())
                bounds = get_bounds(aabb.front());
            else if(!circle.empty())
                bounds = get_bounds(circle.front());
            else if(!triangle.empty())
                bounds = get_bounds(triangle.front());
            bound_limit(aabb);
            bound_limit(circle);
            bound_limit(triangle);
        }
        template <typename S> ColliderCollection<S> transform(const std::function<Vector<S, 2>(const Vector<T, 2>&)>& transformFunc, const std::function<S(const T&)>& transformLinearFunc) const {
            ColliderCollection<S> toRet;
            for(auto& o : aabb)
                toRet.aabb.emplace_back(o.transform(transformFunc, transformLinearFunc));
            for(auto& o : circle)
                toRet.circle.emplace_back(o.transform(transformFunc, transformLinearFunc));
            for(auto& o : triangle)
                toRet.triangle.emplace_back(o.transform(transformFunc, transformLinearFunc));
            toRet.recalculate_bounds();
            return toRet;
        }
    };

    template <typename T> std::ostream& operator<<(std::ostream& os, const AABB<T>& a) {
        return os << "min: [" << a.min.x() << " " << a.min.y() << "]\n"
                  << "max: [" << a.max.x() << " " << a.max.y() << "]\n";
    }

    template <typename T> const AABB<T>& get_bounds(const Circle<T>& c) { return c.bounds; }
    template <typename T> const AABB<T>& get_bounds(const Triangle<T>& c) { return c.bounds; }
    template <typename T> const AABB<T>& get_bounds(const AABB<T>& a) { return a; }
    template <typename T> const AABB<T>& get_bounds(const ColliderCollection<T>& c) { return c.bounds; }

    template<typename T> void generate_wide_line(ColliderCollection<T>& collection, const Vector<T, 2>& p1, const Vector<T, 2>& p2, T width, bool circleCaps) {
        if(p1 != p2) {
            Vector<T, 2> dir = p1 - p2;
            Vector<T, 2> perpVec = perpendicular_vec2(dir);
            perpVec.normalize();
            Vector<T, 2> perpWidth = (perpVec * width * T(0.5)).eval();
            collection.triangle.emplace_back(Triangle<T>{p1 + perpWidth, p1 - perpWidth, p2 + perpWidth});
            collection.triangle.emplace_back(Triangle<T>{p2 + perpWidth, p2 - perpWidth, p1 - perpWidth});
        }
        if(circleCaps) {
            collection.circle.emplace_back(Circle<T>{p1, width * T(0.5)});
            if(p1 != p2)
                collection.circle.emplace_back(Circle<T>{p2, width * T(0.5)});
        }
        collection.recalculate_bounds();
    }

    template <typename T> Vector<T, 2> two_dim_vec_slerp(const Vector<T, 2>& v1, const Vector<T, 2>& v2) {
        Vector<T, 2> v1Norm = v1.normalized();
        Vector<T, 2> v2Norm = v2.normalized();
        Vector<T, 2> avg = (v1Norm + v2Norm) * 0.5;
        if(std::abs(avg.x()) < 0.00001 && std::abs(avg.y()) < 0.00001)
            return perpendicular_vec2(v1Norm).normalized();
        else
            return avg.normalized();
    }

    template <typename T> void generate_polyline(ColliderCollection<T>& collection, const std::vector<Vector<T, 2>>& points, T width, bool closePath) {
        if(points.size() < 2) {
            return;
        }

        Vector<T, 2> perpPrev = perpendicular_vec2((points[1] - points.front()).eval());
        perpPrev.normalize();

        std::array<Vector<T, 2>, 3> vertArray;
        unsigned newIndex = 0;

        if(!closePath || points.size() == 2) {
            vertArray[0] = points.front() + perpPrev * width * 0.5;
            vertArray[1] = points.front() - perpPrev * width * 0.5;
            newIndex = 2;
        }

        if(points.size() == 2) {
            vertArray[newIndex] = points.back() + perpPrev * width * 0.5;
            collection.triangle.emplace_back(Triangle<T>{vertArray});
            newIndex = (newIndex + 1) % 3;
            vertArray[newIndex] = points.back() - perpPrev * width * 0.5;
            collection.triangle.emplace_back(Triangle<T>{vertArray});
            return;
        }

        for(size_t x = 0; x <= points.size(); x++) {
            size_t i = x;
            size_t iP1 = x + 1;
            size_t iM1 = x - 1;
            if(x == 0) {
                if(!closePath)
                    continue;
                else
                    iM1 = points.size() - 1;
            }
            else if(x == points.size() - 1) {
                if(!closePath)
                    continue;
                else {
                    iP1 = 0;
                }
            }
            else if(x == points.size()) {
                if(!closePath)
                    continue;
                else {
                    i = 0;
                    iM1 = points.size() - 1;
                    iP1 = 1;
                }
            }
            Vector<T, 2> v1Norm = (points[iM1] - points[i]).normalized();
            Vector<T, 2> v2Norm = (points[iP1] - points[i]).normalized();
            Vector<T, 2> perp = two_dim_vec_slerp(v1Norm, v2Norm);
            if(perp.dot(perpPrev) <= 0.0)
                perp = -perp;

            T angle1 = std::acos((perp.dot(v1Norm) < 0 ? -perp : perp).dot(v1Norm));
            T angle2 = std::acos((perp.dot(v2Norm) < 0 ? -perp : perp).dot(v2Norm));
            T sinAngle1 = std::sin(angle1 + angle2);
            T alteredWidth = std::lerp(0.5, 0.707, sinAngle1);

            vertArray[newIndex] = points[i] + perp * alteredWidth * width;
            newIndex = (newIndex + 1) % 3;
            if(x != 0)
                collection.triangle.emplace_back(Triangle<T>{vertArray});
            vertArray[newIndex] = points[i] - perp * alteredWidth * width;
            newIndex = (newIndex + 1) % 3;
            if(x != 0)
                collection.triangle.emplace_back(Triangle<T>{vertArray});

            perpPrev = perp;
        }

        if(!closePath) {
            Vector<T, 2> perp = perpendicular_vec2((points[points.size() - 2] - points.back()).eval());
            perp.normalize();
            if(perp.dot(perpPrev) <= 0.0)
                perp = -perp;
            vertArray[newIndex] = points.back() + perp * width * 0.5;
            newIndex = (newIndex + 1) % 3;
            collection.triangle.emplace_back(Triangle<T>{vertArray});
            vertArray[newIndex] = points.back() - perp * width * 0.5;
            newIndex = (newIndex + 1) % 3;
            collection.triangle.emplace_back(Triangle<T>{vertArray});
        }
    }

    // https://blackpawn.com/texts/pointinpoly/default.html
    template <typename T> bool collide(const Vector<T, 2>& p, const Triangle<T>& t) {
        Vector<T, 2> v0 = t.p[2] - t.p[0];
        Vector<T, 2> v1 = t.p[1] - t.p[0];
        Vector<T, 2> v2 = p - t.p[0];

        T dot00 = v0.dot(v0);
        T dot01 = v0.dot(v1);
        T dot02 = v0.dot(v2);
        T dot11 = v1.dot(v1);
        T dot12 = v1.dot(v2);
        
        T invDenom = T(1) / (dot00 * dot11 - dot01 * dot01);
        T u = (dot11 * dot02 - dot01 * dot12) * invDenom;
        T v = (dot00 * dot12 - dot01 * dot02) * invDenom;

        return (u >= T(0)) && (v >= T(0)) && (u + v < T(1));
    }
    template <typename T> bool collide(const Triangle<T>& t, const Vector<T, 2>& p) {
        return collide(p, t);
    }
    template <typename T> bool collide(const Vector<T, 2>& p, const AABB<T>& a) {
        return (p.x() > a.min.x() && p.x() < a.max.x() && p.y() > a.min.y() && p.y() < a.max.y());
    }
    template <typename T> bool collide(const AABB<T>& a, const Vector<T, 2>& p) {
        return collide(p, a);
    }
    template <typename T> bool collide(const Vector<T, 2>& p, const Circle<T>& a) {
        return vec_distance(p, a.pos) <= a.radius;
    }
    template <typename T> bool collide(const Circle<T>& a, const Vector<T, 2>& p) {
        return collide(p, a);
    }

    template <typename T> bool collide(const Circle<T>& c1, const Circle<T>& c2) {
        return vec_distance(c1.pos, c2.pos) <= (c1.radius + c2.radius);
    }

    // https://gamedev.stackexchange.com/a/178154
    template <typename T> bool collide(const Circle<T>& c, const AABB<T>& a) {
        if(!collide(get_bounds(c), a))
            return false;
        Vector<T, 2> aabbCenter = a.center();
        Vector<T, 2> aabbBounds = aabbCenter - a.min;
        Vector<T, 2> distance = c.pos - aabbCenter;
        Vector<T, 2> clampDst = cwise_vec_clamp(distance, (-aabbBounds).eval(), aabbBounds);
        Vector<T, 2> closestPoint = aabbCenter + clampDst;
        return vec_distance(closestPoint, c.pos) <= c.radius;
    }
    template <typename T> bool collide(const AABB<T>& a, const Circle<T>& c) {
        return collide(c, a);
    }

    template <typename T> bool collide(const AABB<T>& a1, const AABB<T>& a2) {
        return a1.min.x() < a2.max.x() && a1.max.x() > a2.min.x() &&
               a1.min.y() < a2.max.y() && a1.max.y() > a2.min.y();
    }

    template <typename T> bool collide(const Circle<T>& c, const Triangle<T>& t) {
        if(!collide(get_bounds(c), get_bounds(t)))
            return false;
        return collide(t.p[0], c) ||
               collide(t.p[1], c) ||
               collide(t.p[2], c) ||
               collide(c.pos, t) ||
               collision_circle_line_segment(c.pos, c.radius, t.p[0], t.p[1]) ||
               collision_circle_line_segment(c.pos, c.radius, t.p[1], t.p[2]) ||
               collision_circle_line_segment(c.pos, c.radius, t.p[2], t.p[0]);
    }
    template <typename T> bool collide(const Triangle<T>& t, const Circle<T>& c) {
        return collide(c, t);
    }

    template <typename T> bool collide(const AABB<T>& a, const Triangle<T>& t) {
        if(!collide(a, get_bounds(t)))
            return false;
        return collide(t.p[0], a) ||
               collide(t.p[1], a) ||
               collide(t.p[2], a) ||
               collide(a.min, t) ||
               collide(a.max, t) ||
               collide(a.bottom_left(), t) ||
               collide(a.top_right(), t) ||
               is_collision_line_segment_line_segment(t.p[0], t.p[1], a.min, a.top_right()) ||
               is_collision_line_segment_line_segment(t.p[0], t.p[1], a.min, a.bottom_left()) ||
               is_collision_line_segment_line_segment(t.p[0], t.p[1], a.max, a.top_right()) ||
               is_collision_line_segment_line_segment(t.p[0], t.p[1], a.max, a.bottom_left()) ||
               is_collision_line_segment_line_segment(t.p[0], t.p[2], a.min, a.top_right()) ||
               is_collision_line_segment_line_segment(t.p[0], t.p[2], a.min, a.bottom_left()) ||
               is_collision_line_segment_line_segment(t.p[0], t.p[2], a.max, a.top_right()) ||
               is_collision_line_segment_line_segment(t.p[0], t.p[2], a.max, a.bottom_left()) ||
               is_collision_line_segment_line_segment(t.p[2], t.p[1], a.min, a.top_right()) ||
               is_collision_line_segment_line_segment(t.p[2], t.p[1], a.min, a.bottom_left()) ||
               is_collision_line_segment_line_segment(t.p[2], t.p[1], a.max, a.top_right()) ||
               is_collision_line_segment_line_segment(t.p[2], t.p[1], a.max, a.bottom_left());
    }
    template <typename T> bool collide(const Triangle<T>& t, const AABB<T>& a) {
        return collide(a, t);
    }

    template <typename T> bool collide(const Triangle<T>& t1, const Triangle<T>& t2) {
        //return collide(get_bounds(t1), get_bounds(t2));
        if(!collide(get_bounds(t1), get_bounds(t2)))
            return false;
        return collide(t1.p[0], t2) ||
               collide(t1.p[1], t2) ||
               collide(t1.p[2], t2) ||
               collide(t2.p[0], t1) ||
               collide(t2.p[1], t1) ||
               collide(t2.p[2], t1) ||
               is_collision_line_segment_line_segment(t1.p[0], t1.p[1], t2.p[0], t2.p[1]) ||
               is_collision_line_segment_line_segment(t1.p[0], t1.p[1], t2.p[1], t2.p[2]) ||
               is_collision_line_segment_line_segment(t1.p[0], t1.p[1], t2.p[2], t2.p[0]) ||
               is_collision_line_segment_line_segment(t1.p[0], t1.p[2], t2.p[0], t2.p[1]) ||
               is_collision_line_segment_line_segment(t1.p[0], t1.p[2], t2.p[1], t2.p[2]) ||
               is_collision_line_segment_line_segment(t1.p[0], t1.p[2], t2.p[2], t2.p[0]) ||
               is_collision_line_segment_line_segment(t1.p[2], t1.p[1], t2.p[0], t2.p[1]) ||
               is_collision_line_segment_line_segment(t1.p[2], t1.p[1], t2.p[1], t2.p[2]) ||
               is_collision_line_segment_line_segment(t1.p[2], t1.p[1], t2.p[2], t2.p[0]);
    }

    template <typename Collection, typename Collider> bool collide(const Collection& collection, const Collider& c) {
        for(auto& o : collection.circle)
            if(collide(c, o))
                return true;
        for(auto& o : collection.aabb)
            if(collide(c, o))
                return true;
        for(auto& o : collection.triangle)
            if(collide(c, o))
                return true;
        return false;
    }

    template <typename T> class BVHContainer {
        public:
            typedef T UnderlyingType;

            std::vector<BVHContainer<T>> children;
            ColliderCollection<T> objects;
            
            // NOTE: BVH TRANSFORMATION LEADS TO A BROKEN BVH
            //void transform(const std::function<Vector<T, 2>(const Vector<T, 2>&)>& transformFunc, const std::function<T(const T&)>& transformLinearFunc) {
            //    objects.transform(transformFunc, transformLinearFunc);
            //    for(auto& a : children)
            //        a.transform(transformFunc, transformLinearFunc);
            //}

            void clear() {
                children.clear();
                objects.clear();
            }

            bool empty() const {
                return children.empty() && objects.empty();
            }

            unsigned assign_quad_to_point(const Vector<T, 2>& point, const Vector<T, 2>& origin) const {
                bool negativeX = point.x() <= origin.x();
                bool negativeY = point.y() <= origin.y();

                if(negativeX && negativeY)
                    return 0;
                else if(negativeX && !negativeY)
                    return 1;
                else if(!negativeX && !negativeY)
                    return 2;
                else
                    return 3;
            }

            void calculate_bvh_recursive(ColliderCollection<T>& bvhObjects, int maxDepth = -1) {
                bvhObjects.recalculate_bounds();

                objects.bounds = bvhObjects.bounds;

                if(bvhObjects.empty()) {
                    return;
                }

                if(bvhObjects.size() <= 2 || maxDepth == 0) {
                    objects = bvhObjects;
                    return;
                }

                ColliderCollection<T> parts[4];
                Vector<T, 2> halfwayPoint = bvhObjects.bounds.center();

                for(auto& o : bvhObjects.aabb)
                    parts[assign_quad_to_point(get_bounds(o).center(), halfwayPoint)].aabb.emplace_back(o);
                for(auto& o : bvhObjects.circle)
                    parts[assign_quad_to_point(get_bounds(o).center(), halfwayPoint)].circle.emplace_back(o);
                for(auto& o : bvhObjects.triangle)
                    parts[assign_quad_to_point(get_bounds(o).center(), halfwayPoint)].triangle.emplace_back(o);

                for(size_t i = 0; i < 4; i++) {
                    if(parts[i].empty())
                        continue;
                    else if(parts[i].size() == bvhObjects.size()) {
                        objects = bvhObjects;
                        break;
                    }
                    else {
                        children.emplace_back();
                        children.back().calculate_bvh_recursive(parts[i], maxDepth - 1);
                    }
                }
            }

            template <typename Collider> bool is_collide(const Collider& collider) const {
                if(!collide(get_bounds(collider), get_bounds(objects)))
                    return false;
                if(collide(objects, collider))
                    return true;
                for(const auto& c : children)
                    if(c.is_collide(collider))
                        return true;
                return false;
            }

            template <typename BVH> bool is_collide_other_bvh(const BVH& bvh) const {
                if(!collide(get_bounds(objects), get_bounds(bvh.objects)))
                    return false;
                if(is_collide(bvh.objects))
                    return true;
                if(bvh.is_collide(objects))
                    return true;
                for(const auto& c : children) {
                    for(const auto& d : bvh.children) {
                        if(c.is_collide_other_bvh(d))
                            return true;
                    }
                }
                return false;
            }
    };
};

