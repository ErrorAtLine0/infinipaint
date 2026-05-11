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

#include "FileHelpers.hpp"
#include <Helpers/StringHelpers.hpp>
#include <algorithm>

std::filesystem::path force_extension_on_path(std::filesystem::path p, const std::string& extensions) {
    std::vector<std::string> extensionList = split_string_by_token(extensions, ";");
    std::erase(extensionList, "*");
    if(extensionList.empty())
        return p;
    for(std::string& s : extensionList) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        s.insert(0, ".");
    }
    

    std::string fExtension;
    if(p.has_extension()) {
        fExtension = p.extension().string();
        std::transform(fExtension.begin(), fExtension.end(), fExtension.begin(), ::tolower);
    }

    if(std::ranges::contains(extensionList, fExtension))
        return p;

    p += extensionList[0];
    return p;
}
