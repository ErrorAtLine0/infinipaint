#pragma once
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <cereal/types/utility.hpp>
#include <Helpers/Hashes.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <Helpers/FixedPoint.hpp>

using namespace Eigen;

typedef Vector<WorldScalar, 2> WorldVec;

enum DrawComponentType : uint8_t {
    DRAWCOMPONENT_BRUSHSTROKE = 0,
    DRAWCOMPONENT_RECTANGLE,
    DRAWCOMPONENT_ELLIPSE,
    DRAWCOMPONENT_TEXTBOX,
    DRAWCOMPONENT_IMAGE
};

typedef uint64_t ClientPortionID;
typedef uint64_t ServerPortionID;
typedef std::pair<ServerPortionID, ClientPortionID> ServerClientID;

template <> struct std::hash<ServerClientID> {
    size_t operator()(const ServerClientID& v) const
    {
        uint64_t h = 0;
        hash_combine(h, v.first, v.second);
        return h;
    }
};

template <typename F, typename S> std::ostream& operator<<(std::ostream& os, const std::pair<F, S>& p) {
    os << p.first << ", " << p.second;
    return os;
}
