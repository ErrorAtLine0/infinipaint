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

#include "Random.hpp"

Random Random::global;

Random& Random::get() {
    return global;
}

std::string Random::alphanumeric_str(size_t len) {
    std::string toRet;
    char allowedChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t allowedCharCount = sizeof(allowedChars) / sizeof(char);

    // We use allowedCharCount - 2 as the right bound because the range is inclusive,
    // and we dont want to include the implicit null terminator in the end of allowedChars
    // as a possible character
    for(size_t i = 0; i < len; i++)
        toRet.push_back(allowedChars[Random::get().int_range<size_t>(0, allowedCharCount - 2)]);

    return toRet;
}

Random::Random():
    randGen(std::random_device()())
{
}
