#include "CoordSpaceHelper.hpp"
#include "MainProgram.hpp"
#include <Helpers/MathExtras.hpp>
#include "World.hpp"
#include "MainProgram.hpp"

CoordSpaceHelper::CoordSpaceHelper() { }

CoordSpaceHelper::CoordSpaceHelper(const WorldVec& initPos, const WorldScalar& initInverseScale, double initRotation):
    pos(initPos),
    inverseScale(initInverseScale),
    rotation(initRotation)
{}

WorldVec rotate_world_coord(const WorldVec& a, double rotationAngle) {
    FixedPoint::Number<boost::multiprecision::cpp_int, 15> s = FixedPoint::Number<boost::multiprecision::cpp_int, 15>(std::sin(rotationAngle));
    FixedPoint::Number<boost::multiprecision::cpp_int, 15> c = FixedPoint::Number<boost::multiprecision::cpp_int, 15>(std::cos(rotationAngle));
    return {
        a.x().multiply_different_precision(c) - a.y().multiply_different_precision(s),
        a.x().multiply_different_precision(s) + a.y().multiply_different_precision(c)
    };
}

Vector2f CoordSpaceHelper::to_space(const WorldVec& coord) const {
    return (Rotation2D<double>(-rotation) * ((coord - pos) / inverseScale).cast<double>()).cast<float>();
}

WorldVec CoordSpaceHelper::from_space(Vector2f coord) const {
    return ((Rotation2D<double>(rotation) * coord.cast<double>()).cast<WorldScalar>() * inverseScale + pos);
}

WorldVec CoordSpaceHelper::to_space_world(const WorldVec& coord) const {
    return rotate_world_coord(((coord - pos) / inverseScale), -rotation);
}

WorldVec CoordSpaceHelper::from_space_world(const WorldVec& coord) const {
    return rotate_world_coord(coord, rotation) * inverseScale + pos;
}

WorldScalar CoordSpaceHelper::scalar_from_space(float coord) const {
    return WorldScalar(coord) * inverseScale;
}

float CoordSpaceHelper::scalar_to_space(const WorldScalar& coord) const {
    return static_cast<float>(coord / inverseScale);
}

WorldVec CoordSpaceHelper::dir_from_space(Vector2f coord) const {
    return (Rotation2D<double>(rotation) * coord.cast<double>()).cast<WorldScalar>() * inverseScale;
}

Vector2f CoordSpaceHelper::dir_to_space(const WorldVec& coord) const {
    return (Rotation2D<double>(-rotation) * (coord / inverseScale).cast<double>()).cast<float>();
}

Vector2f CoordSpaceHelper::normalized_dir_to_space(const WorldVec& coord) const {
    return (Rotation2D<double>(-rotation) * coord.cast<double>()).cast<float>();
}

void CoordSpaceHelper::scale_about_double(const WorldVec& scalePos, double scaleAmount) {
    if(scaleAmount > 1.0) {
        WorldScalar worldScale(scaleAmount);
        inverseScale /= worldScale;
        WorldVec mVec = pos - scalePos;
        pos = scalePos + mVec / worldScale;
    }
    else {
        WorldScalar worldScale(1.0 / scaleAmount);
        inverseScale *= worldScale;
        WorldVec mVec = pos - scalePos;
        pos = scalePos + mVec * worldScale;
    }
}

void CoordSpaceHelper::scale_about(const WorldVec& scalePos, const WorldScalar& scaleAmount, bool flipScale) {
    if(!flipScale) {
        inverseScale /= scaleAmount;
        WorldVec mVec = pos - scalePos;
        pos = scalePos + mVec / scaleAmount;
    }
    else {
        inverseScale *= scaleAmount;
        WorldVec mVec = pos - scalePos;
        pos = scalePos + mVec * scaleAmount;
    }
}

void CoordSpaceHelper::rotate_about(const WorldVec& rotatePos, double angle) {
    WorldVec a = pos - rotatePos;
    FixedPoint::Number<boost::multiprecision::cpp_int, 15> s = FixedPoint::Number<boost::multiprecision::cpp_int, 15>(std::sin(angle));
    FixedPoint::Number<boost::multiprecision::cpp_int, 15> c = FixedPoint::Number<boost::multiprecision::cpp_int, 15>(std::cos(angle));
    set_rotation(rotation + angle);
    pos = {
        rotatePos.x() + a.x().multiply_different_precision(c) - a.y().multiply_different_precision(s),
        rotatePos.y() + a.x().multiply_different_precision(s) + a.y().multiply_different_precision(c)
    };
}

void CoordSpaceHelper::translate(const WorldVec& translation) {
    pos += translation;
}

void CoordSpaceHelper::scale(const WorldScalar& scaleAmount) {
    inverseScale /= scaleAmount;
}

void CoordSpaceHelper::set_rotation(double newRotation) {
    rotation = newRotation;
    // using circular fmod caused inaccuracies, so these loops might be a better solution
    while(rotation >= 2.0 * std::numbers::pi)
        rotation -= 2.0 * std::numbers::pi;
    while (rotation < 0.0)
        rotation += 2.0 * std::numbers::pi;
}

std::vector<Vector2f> CoordSpaceHelper::to_space(const std::vector<WorldVec>& coord) const {
    std::vector<Vector2f> toRet(coord.size());
    for(size_t i = 0; i < coord.size(); i++)
        toRet[i] = to_space(coord[i]);
    return toRet;
}

std::vector<WorldVec> CoordSpaceHelper::from_space(const std::vector<Vector2f>& coord) const {
    std::vector<WorldVec> toRet(coord.size());
    for(size_t i = 0; i < coord.size(); i++)
        toRet[i] = from_space(coord[i]);
    return toRet;
}

std::vector<WorldVec> CoordSpaceHelper::to_space_world(const std::vector<WorldVec>& coord) const {
    std::vector<WorldVec> toRet(coord.size());
    for(size_t i = 0; i < coord.size(); i++)
        toRet[i] = to_space_world(coord[i]);
    return toRet;
}

std::vector<WorldVec> CoordSpaceHelper::from_space_world(const std::vector<WorldVec>& coord) const {
    std::vector<WorldVec> toRet(coord.size());
    for(size_t i = 0; i < coord.size(); i++)
        toRet[i] = from_space_world(coord[i]);
    return toRet;
}

Vector2f CoordSpaceHelper::get_mouse_pos(const World& w) const {
    return to_space(w.drawData.cam.c.from_space(w.main.input.mouse.pos));
}

CoordSpaceHelper CoordSpaceHelper::other_coord_space_to_this_space(const CoordSpaceHelper& other) const {
    CoordSpaceHelper toRet;
    toRet.pos = to_space_world(other.pos);
    toRet.rotation = other.rotation - rotation;
    toRet.inverseScale = other.inverseScale / inverseScale;
    return toRet;
}

CoordSpaceHelper CoordSpaceHelper::other_coord_space_from_this_space(const CoordSpaceHelper& other) const {
    CoordSpaceHelper toRet;
    toRet.pos = from_space_world(other.pos);
    toRet.rotation = other.rotation + rotation;
    toRet.inverseScale = other.inverseScale * inverseScale;
    return toRet;
}

void CoordSpaceHelper::transform_sk_canvas(SkCanvas* canvas, const DrawData& drawData) const {
    Vector2f translateSpace = -to_space(drawData.cam.c.pos);
    // NOTE: Using this commented code will lead to more accurate results with small scales, but that isn't really required in this case since that stuff just wouldn't be rendered
    //
    //double aS;
    //if(inverseScale > drawData.cam.c.inverseScale)
    //    aS = static_cast<double>(inverseScale / drawData.cam.c.inverseScale);
    //else
    //    aS = 1.0 / static_cast<double>(drawData.cam.c.inverseScale / inverseScale);
    double aS = static_cast<double>(inverseScale / drawData.cam.c.inverseScale);

    canvas->scale(aS, aS);
    canvas->rotate((rotation - drawData.cam.c.rotation) * 180.0 / std::numbers::pi);
    canvas->translate(translateSpace.x(), translateSpace.y());
}
