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
#include <clay.h>

#ifdef IS_CLIENT
    #include <include/core/SkPoint.h>
    template <typename S> S convert_vec2(const SkPoint& t) {
        return S{t.fX, t.fY};
    }
#endif

template <typename S, typename T> S convert_vec2(const T& t) {
    return S{t[0], t[1]};
}

template <typename S, typename T> S convert_vec3(const T& t) {
    return S{t[0], t[1], t[2]};
}

template <typename S, typename T> S convert_vec4(const T& t) {
    return S{t[0], t[1], t[2], t[3]};
}

template <typename S> S convert_vec4(const Clay_Color& t) {
    return S{t.r, t.g, t.b, t.a};
}

template <typename S> S convert_vec2(const Clay_Dimensions& t) {
    return S{t.width, t.height};
}
