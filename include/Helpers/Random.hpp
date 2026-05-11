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
#include <random>

class Random {
    public:
        static Random& get();
        Random();
        template <typename T> T int_range(T start, T end) {
            std::uniform_int_distribution<T> distribute(start, end);
            return distribute(randGen);
        }
        template <typename T> T real_range(T start, T end) {
            std::uniform_real_distribution<T> distribute(start, end);
            return distribute(randGen);
        }
        std::string alphanumeric_str(size_t len);
        std::mt19937_64 randGen;
    private:
        static Random global;
};
