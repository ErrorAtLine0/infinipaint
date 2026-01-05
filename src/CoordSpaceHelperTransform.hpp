#pragma once
#include "SharedTypes.hpp"
#include <Helpers/Serializers.hpp>
#include <array>
#include <Helpers/SCollision.hpp>
#include "CoordSpaceHelper.hpp"

#ifndef IS_SERVER
#include "include/core/SkMatrix.h"
#include "include/core/SkCanvas.h"
#endif

class World;
struct DrawData;

class CoordSpaceHelperTransform {
    public:
        CoordSpaceHelperTransform();
        CoordSpaceHelperTransform(const WorldVec& translationPos);
        CoordSpaceHelperTransform(const WorldVec& rotatePos, double angle);
        CoordSpaceHelperTransform(const WorldVec& scalePos, const WorldMultiplier& inverseScaleAmount);

        WorldVec from_space_world(const WorldVec& coord) const;
        //WorldVec to_space_world(const WorldVec& coord) const;

        CoordSpaceHelper other_coord_space_to_this_space(const CoordSpaceHelper& other) const;
        CoordSpaceHelper other_coord_space_from_this_space(const CoordSpaceHelper& other) const;

        bool operator==(const CoordSpaceHelperTransform& otherCoords) const;
        bool operator!=(const CoordSpaceHelperTransform& otherCoords) const;

        enum class TransformType {
            NONE,
            TRANSLATE,
            ROTATE,
            SCALE
        };

        bool is_identity();

    private:
        void set_rotation(double newRotation);
        void translate(const WorldVec& translation);
        void rotate_about(const WorldVec& rotatePos, double angle);
        void scale_about(const WorldVec& scalePos, const WorldMultiplier& scaleAmount);

        TransformType transformType = TransformType::NONE;
        WorldVec pos{0, 0}; // rotation center, scale center, or translation value
        WorldMultiplier inverseScale{1};
        double rotation = 0.0;
};

std::ostream& operator<<(std::ostream& os, const CoordSpaceHelperTransform& a);

