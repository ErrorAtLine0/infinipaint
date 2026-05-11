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

#include "VersionConstants.hpp"
#include <stdexcept>

namespace VersionConstants {

VersionNumber header_to_version_number(const std::string& header) {
    static std::unordered_map<std::string, VersionNumber> m;
    if(m.empty()) {
        m["INFPNT000001"] = VersionNumber(0, 0, 1);
        m["INFPNT000002"] = VersionNumber(0, 1, 0);
        m["INFPNT000003"] = VersionNumber(0, 2, 0);
        m["INFPNT000004"] = VersionNumber(0, 3, 0);
        m["INFPNT000005"] = VersionNumber(0, 4, 0);
    }
    auto it = m.find(header);
    if(it == m.end())
        throw std::runtime_error("[VersionConstants::header_to_version_number] Header " + header + " is invalid");
    return it->second;
}

}
