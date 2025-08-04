#include "CoordSpaceHelperTransform.hpp"

bool CoordSpaceHelperTransform::operator==(const CoordSpaceHelperTransform& otherCoords) const {
    return rotation == otherCoords.rotation && pos == otherCoords.pos && inverseScale == otherCoords.inverseScale;
}

bool CoordSpaceHelperTransform::operator!=(const CoordSpaceHelperTransform& otherCoords) const {
    return rotation != otherCoords.rotation || pos != otherCoords.pos || inverseScale != otherCoords.inverseScale;
}

WorldVec CoordSpaceHelperTransform::to_space_world(const WorldVec& coord) const {
    return rotate_world_coord(FixedPoint::multiplier_vec_div((coord - pos), inverseScale), -rotation);
}

WorldVec CoordSpaceHelperTransform::from_space_world(const WorldVec& coord) const {
    return FixedPoint::multiplier_vec_mult(rotate_world_coord(coord, rotation), inverseScale) + pos;
}

CoordSpaceHelper CoordSpaceHelperTransform::other_coord_space_to_this_space(const CoordSpaceHelper& other) const {
    CoordSpaceHelper toRet;
    toRet.pos = to_space_world(other.pos);
    toRet.rotation = other.rotation - rotation;
    toRet.inverseScale = other.inverseScale / inverseScale;
    return toRet;
}

CoordSpaceHelper CoordSpaceHelperTransform::other_coord_space_from_this_space(const CoordSpaceHelper& other) const {
    CoordSpaceHelper toRet;
    toRet.pos = from_space_world(other.pos);
    toRet.rotation = other.rotation + rotation;
    toRet.inverseScale = other.inverseScale * inverseScale;
    return toRet;
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

void CoordSpaceHelperTransform::scale_about(const WorldVec& scalePos, const WorldMultiplier& scaleAmount) {
    inverseScale = inverseScale / scaleAmount;
    WorldVec mVec = pos - scalePos;
    pos = scalePos + FixedPoint::multiplier_vec_div(mVec, scaleAmount);
}

std::ostream& operator<<(std::ostream& os, const CoordSpaceHelperTransform& a) {
    return os << "pos: [" << a.pos.x() << " " << a.pos.y() << "]\n"
              << "inverseScale: " << static_cast<WorldScalar>(a.inverseScale) << "\n"
              << "rotation: " << a.rotation << "\n";
}
