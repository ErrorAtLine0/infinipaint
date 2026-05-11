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
