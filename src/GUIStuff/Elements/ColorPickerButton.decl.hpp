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

namespace GUIStuff {

struct ColorPickerButtonData {
    bool hasAlpha = true;
    std::function<void()> onEdit;
    std::function<void()> onSelect;
    std::function<void()> onDeselect;
};

template <typename T> class ColorPickerButton : public Element {
    public:
        ColorPickerButton(GUIManager& gui);
        void layout(const Clay_ElementId& id, T* val, const ColorPickerButtonData& d);
    private:
        ColorPickerButtonData data;
        bool isOpen = false;
};

}
