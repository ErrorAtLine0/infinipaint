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
#include <stack>
#include <Helpers/Hashes.hpp>

namespace GUIStuff {

struct GUIManagerID {
    GUIManagerID() = default;
    GUIManagerID(intptr_t numID);
    GUIManagerID(const char* strID);
    bool operator==(const GUIManagerID& other) const;

    bool idIsStr;
    intptr_t idNum;
};

typedef std::vector<GUIManagerID> GUIManagerIDStack;

}

template <> struct std::hash<GUIStuff::GUIManagerID> {
    size_t operator()(const GUIStuff::GUIManagerID& v) const
    {
        return std::hash<intptr_t>{}(v.idNum);
    }
};

template <> struct std::hash<GUIStuff::GUIManagerIDStack> {
    size_t operator()(const GUIStuff::GUIManagerIDStack& v) const
    {
        uint64_t h = 0;
        for(const auto& i : v)
            hash_combine(h, i);
        return h;
    }
};
