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
#include <cstdint>
#include <array>
#include <Helpers/Random.hpp>
#include <Helpers/Hashes.hpp>

namespace NetworkingObjects {
    struct NetObjID {
        std::array<uint64_t, 2> data;
        template <typename Archive> void serialize(Archive& a) {
            a(data[1], data[0]);
        }
        std::string to_string() const {
            std::stringstream ss;
            ss << "[" << data[1] << ", " << data[0] << "]";
            return ss.str();
        }
        NetObjID& operator=(const NetObjID&) = default;
        bool operator==(const NetObjID&) const = default;
        static NetObjID random_gen();
    };
};

template <> struct std::hash<NetworkingObjects::NetObjID> {
    size_t operator()(const NetworkingObjects::NetObjID& v) const
    {
        uint64_t h = 0;
        hash_combine(h, v.data[1], v.data[0]);
        return h;
    }
};
