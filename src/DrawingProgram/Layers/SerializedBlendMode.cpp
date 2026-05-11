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

#include "SerializedBlendMode.hpp"
#include <unordered_set>

SkBlendMode serialized_blend_mode_to_sk_blend_mode(SerializedBlendMode serializedBlendMode) {
    int skBlendModeStartVal = static_cast<int>(SkBlendMode::kClear);
    return static_cast<SkBlendMode>(skBlendModeStartVal + static_cast<uint8_t>(serializedBlendMode));
}

const std::vector<SerializedBlendMode>& get_blend_mode_useful_list() {
    static std::vector<SerializedBlendMode> blendModeList;
    if(blendModeList.empty()) {
        std::unordered_set<SerializedBlendMode> exclusionList = {SerializedBlendMode::BLEND_CLEAR, SerializedBlendMode::BLEND_SRC, SerializedBlendMode::BLEND_DST};
        for(uint8_t i = 0; i < static_cast<uint8_t>(SerializedBlendMode::BLEND_LAST_MODE); i++) {
            if(!exclusionList.contains(static_cast<SerializedBlendMode>(i)))
                blendModeList.emplace_back(static_cast<SerializedBlendMode>(i));
        }
    }
    return blendModeList;
}

const std::vector<std::string>& get_blend_mode_useful_name_list() {
    static std::vector<std::string> blendModeNameList;
    if(blendModeNameList.empty()) {
        for(SerializedBlendMode i : get_blend_mode_useful_list())
            blendModeNameList.emplace_back(nlohmann::json(static_cast<SerializedBlendMode>(i)));
    }
    return blendModeNameList;
}
