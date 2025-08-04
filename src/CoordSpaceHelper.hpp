#pragma once
#include "SharedTypes.hpp"
#include <Helpers/Serializers.hpp>
#include <array>
#include <Helpers/SCollision.hpp>

#ifndef IS_SERVER
#include "include/core/SkMatrix.h"
#include "include/core/SkCanvas.h"
#endif

class World;
struct DrawData;

WorldVec rotate_world_coord(const WorldVec& a, double rotationAngle);

class CoordSpaceHelper {
    public:
        template <typename Archive> void save(Archive& a) const {
            a(pos, inverseScale, rotation);
        }
        template <typename Archive> void load(Archive& a) {
            a(pos, inverseScale, rotation);
        }

        CoordSpaceHelper();
        CoordSpaceHelper(const WorldVec& initPos, const WorldScalar& initInverseScale, double initRotation);

        bool operator==(const CoordSpaceHelper& otherCoords) const;
        bool operator!=(const CoordSpaceHelper& otherCoords) const;

        Vector2f to_space(const WorldVec& coord) const;

        WorldVec from_space(Vector2f coord) const;

        WorldVec from_space_world(const WorldVec& coord) const;
        WorldVec to_space_world(const WorldVec& coord) const;

        WorldScalar scalar_from_space(float coord) const;
        float scalar_to_space(const WorldScalar& coord) const;
        WorldVec dir_from_space(Vector2f coord) const;
        Vector2f dir_to_space(const WorldVec& coord) const;
        Vector2f normalized_dir_to_space(const WorldVec& coord) const;

        CoordSpaceHelper other_coord_space_to_this_space(const CoordSpaceHelper& other) const;
        CoordSpaceHelper other_coord_space_from_this_space(const CoordSpaceHelper& other) const;

        template <typename S, typename T> S collider_to_world(const T& collider) const {
            return collider.template transform<WorldScalar>(
                [&coords = *this](const Vector2f& a) {
                    return coords.from_space(a);
                },
                [&coords = *this](const float& a) {
                    return coords.scalar_from_space(a);
            });
        }

        template <typename T> T collider_to_new_coords(const CoordSpaceHelper newCoords, const T& collider) const {
            return collider.template transform<float>(
                [&coords = *this, &newCoords](const Vector2f& a) {
                    return newCoords.to_space(coords.from_space(a));
                },
                [&coords = *this, &newCoords](const float& a) {
                    return newCoords.scalar_to_space(coords.scalar_from_space(a));
            });
        }

        template <typename S, typename T> S world_collider_to_coords(const T& collider) const {
            return collider.template transform<float>(
                [&coords = *this](const WorldVec& a) {
                    return coords.to_space(a);
                },
                [&coords = *this](const WorldScalar& a) {
                    return coords.scalar_to_space(a);
            });
        }

        //float scalar_to_space(WorldScalar coord) const;
        std::vector<Vector2f> to_space(const std::vector<WorldVec>& coord) const;
        std::vector<WorldVec> from_space(const std::vector<Vector2f>& coord) const;
        std::vector<WorldVec> to_space_world(const std::vector<WorldVec>& coord) const;
        std::vector<WorldVec> from_space_world(const std::vector<WorldVec>& coord) const;

        void translate(const WorldVec& translation);
        void scale(const WorldScalar& scaleAmount);
        void rotate_about(const WorldVec& rotatePos, double angle);
        void scale_about(const WorldVec& scalePos, const WorldScalar& scaleAmount, bool flipScale);
        void scale_about_double(const WorldVec& scalePos, double scaleAmount);
        void set_rotation(double newRotation);

        template <size_t S> std::array<Vector2f, S> to_space(const std::array<WorldVec, S>& coord) const {
            std::array<Vector2f, S> toRet;
            for(size_t i = 0; i < coord.size(); i++)
                toRet[i] = to_space(coord[i]);
            return toRet;
        }
        template <size_t S> std::array<WorldVec, S> from_space(const std::array<Vector2f, S>& coord) const {
            std::array<WorldVec, S> toRet;
            for(size_t i = 0; i < coord.size(); i++)
                toRet[i] = from_space(coord[i]);
            return toRet;
        }

        Vector2f get_mouse_pos(const World& w) const;
        //SkMatrix get_sk_matrix_temp(const WorldTransform& tempTransform, const DrawData& drawData) const;
        //SkMatrix get_sk_matrix(const DrawData& drawData) const;
        void transform_sk_canvas(SkCanvas* canvas, const DrawData& drawData) const;

        WorldVec pos{0, 0};
        WorldScalar inverseScale{1};
        double rotation = 0.0;
};
