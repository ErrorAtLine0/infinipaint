#pragma once
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <cereal/types/utility.hpp>
#include <Helpers/Hashes.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <Helpers/FixedPoint.hpp>

using namespace Eigen;

typedef Vector<WorldScalar, 2> WorldVec;

typedef uint64_t NetClientID;

template <typename F, typename S> std::ostream& operator<<(std::ostream& os, const std::pair<F, S>& p) {
    os << p.first << ", " << p.second;
    return os;
}
