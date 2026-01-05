#include "CoordSpaceHelperTransform.hpp"
#include "CoordSpaceHelper.hpp"
#include "nlohmann/json.hpp"

CoordSpaceHelperTransform::CoordSpaceHelperTransform() {}

CoordSpaceHelperTransform::CoordSpaceHelperTransform(const WorldVec& translationPos):
    transformType(TransformType::TRANSLATE),
    pos(translationPos)
{}

CoordSpaceHelperTransform::CoordSpaceHelperTransform(const WorldVec& rotatePos, double angle):
    transformType(TransformType::ROTATE),
    pos(rotatePos),
    rotation(angle)
{}

CoordSpaceHelperTransform::CoordSpaceHelperTransform(const WorldVec& scalePos, const WorldMultiplier& inverseScaleAmount):
    transformType(TransformType::SCALE),
    pos(scalePos),
    inverseScale(inverseScaleAmount)
{}

bool CoordSpaceHelperTransform::operator==(const CoordSpaceHelperTransform& otherCoords) const {
    return otherCoords.transformType == transformType && rotation == otherCoords.rotation && pos == otherCoords.pos && inverseScale == otherCoords.inverseScale;
}

bool CoordSpaceHelperTransform::operator!=(const CoordSpaceHelperTransform& otherCoords) const {
    return rotation != otherCoords.rotation || pos != otherCoords.pos || inverseScale != otherCoords.inverseScale;
}

//WorldVec CoordSpaceHelperTransform::to_space_world(const WorldVec& coord) const {
//    switch(transformType) {
//        case TransformType::NONE:
//            return coord;
//        case TransformType::TRANSLATE:
//            return (coord - pos);
//        case TransformType::SCALE:
//            return pos + FixedPoint::multiplier_vec_div(coord - pos, inverseScale);
//        case TransformType::ROTATE:
//            return rotate_world_coord(coord + pos, -rotation) - pos;
//    }
//    return coord;
//}

bool CoordSpaceHelperTransform::is_identity() {
    switch(transformType) {
        case TransformType::NONE:
            return true;
        case TransformType::TRANSLATE:
            return pos == WorldVec{0, 0};
        case TransformType::SCALE:
            return inverseScale == WorldScalar(1);
        case TransformType::ROTATE:
            return rotation == 0.0;
    }
    return true;
}

WorldVec CoordSpaceHelperTransform::from_space_world(const WorldVec& coord) const {
    switch(transformType) {
        case TransformType::NONE:
            return coord;
        case TransformType::TRANSLATE:
            return (coord + pos);
        case TransformType::SCALE:
            return FixedPoint::multiplier_vec_div(coord - pos, inverseScale) + pos;
        case TransformType::ROTATE:
            return pos + rotate_world_coord(coord - pos, rotation);
    }
    return coord;
}

CoordSpaceHelper CoordSpaceHelperTransform::other_coord_space_to_this_space(const CoordSpaceHelper& other) const {
    switch(transformType) {
        case TransformType::NONE:
            return other;
        case TransformType::TRANSLATE: {
            auto toRet = other;
            toRet.translate(-pos);
            return toRet;
        }
        case TransformType::SCALE: {
            auto toRet = other;
            toRet.scale_about(pos, WorldMultiplier(1) / inverseScale);
            return toRet;
        }
        case TransformType::ROTATE: {
            auto toRet = other;
            toRet.rotate_about(pos, -rotation);
            return toRet;
        }
    }
    return other;
}

CoordSpaceHelper CoordSpaceHelperTransform::other_coord_space_from_this_space(const CoordSpaceHelper& other) const {
    switch(transformType) {
        case TransformType::NONE:
            return other;
        case TransformType::TRANSLATE: {
            auto toRet = other;
            toRet.translate(pos);
            return toRet;
        }
        case TransformType::SCALE: {
            auto toRet = other;
            toRet.scale_about(pos, inverseScale);
            return toRet;
        }
        case TransformType::ROTATE: {
            auto toRet = other;
            toRet.rotate_about(pos, rotation);
            return toRet;
        }
    }
    return other;
}

void CoordSpaceHelperTransform::translate(const WorldVec& translation) {
    pos += translation;
}

void CoordSpaceHelperTransform::rotate_about(const WorldVec& rotatePos, double angle) {
    WorldVec a = pos - rotatePos;
    FixedPoint::Number<boost::multiprecision::cpp_int, 15> s = FixedPoint::Number<boost::multiprecision::cpp_int, 15>(std::sin(angle));
    FixedPoint::Number<boost::multiprecision::cpp_int, 15> c = FixedPoint::Number<boost::multiprecision::cpp_int, 15>(std::cos(angle));
    set_rotation(rotation + angle);
    pos = {
        rotatePos.x() + a.x().multiply_different_precision(c) - a.y().multiply_different_precision(s),
        rotatePos.y() + a.x().multiply_different_precision(s) + a.y().multiply_different_precision(c)
    };
}

void CoordSpaceHelperTransform::set_rotation(double newRotation) {
    rotation = newRotation;
    // using circular fmod caused inaccuracies, so these loops might be a better solution
    while(rotation >= 2.0 * std::numbers::pi)
        rotation -= 2.0 * std::numbers::pi;
    while (rotation < 0.0)
        rotation += 2.0 * std::numbers::pi;
}
