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

#include "MutableTextLabel.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

MutableTextLabel::MutableTextLabel(GUIManager& gui): Element(gui) {}

void MutableTextLabel::layout(const Clay_ElementId& id, const std::string& text, const Clay_TextElementConfig& textConfig) {
    if(text != oldText) {
        oldText = text;
        gui.invalidate_draw_element(this);
    }
    CLAY(id, {
        .layout = {.sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0)}}
    }) {
        CLAY_TEXT(gui.strArena.std_str_to_clay_str(text), textConfig);
    }
}

}
