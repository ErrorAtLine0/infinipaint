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
#include <execution>
#ifdef __APPLE__
    #define POOLSTL_STD_SUPPLEMENT
    #include <poolstl.hpp>
#endif

template <typename ContainerType> void parallel_loop_container(const ContainerType& c, std::function<void(const typename ContainerType::value_type&)> func, bool forceSingleThread = false) {
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
    std::for_each(c.begin(), c.end(), func);
#else
    if(forceSingleThread)
        std::for_each(c.begin(), c.end(), func);
    else
        std::for_each(std::execution::par_unseq, c.begin(), c.end(), func);
#endif
}
