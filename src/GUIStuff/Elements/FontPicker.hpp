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
#include "Element.hpp"
#include <limits>

namespace GUIStuff {

struct FontPickerData {
    std::function<void()> onFontChange;
};

class FontPicker : public Element {
    public:
        FontPicker(GUIManager& gui);
        void layout(const Clay_ElementId& id, std::string* newFontName, const FontPickerData& newData = {});
        static const std::vector<std::string>& sorted_font_list(GUIManager& gui);
    private:
        std::string* fontName;
        FontPickerData data;

        size_t hoveringOver = std::numeric_limits<size_t>::max();

        bool jumpToFontName = false;

        std::optional<std::string> get_valid_font_name();

        bool dropdownOpen = false;
};

}
