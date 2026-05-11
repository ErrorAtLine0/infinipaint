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

#include "TextLabelHelpers.hpp"
#include "../Elements/MutableTextLabel.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff { namespace ElementHelpers {

void text_label_size(GUIManager& gui, std::string_view val, float modifier) {
    CLAY_TEXT(gui.strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(gui.io.theme->frontColor1), .fontSize = static_cast<uint16_t>(gui.io.fontSize * modifier)}));
}

void text_label_color(GUIManager& gui, std::string_view val, const SkColor4f& color) {
    CLAY_TEXT(gui.strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(color), .fontSize = gui.io.fontSize }));
}

void text_label_light(GUIManager& gui, std::string_view val) {
    CLAY_TEXT(gui.strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(gui.io.theme->frontColor2), .fontSize = gui.io.fontSize }));
}

void text_label(GUIManager& gui, std::string_view val) {
    CLAY_TEXT(gui.strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(gui.io.theme->frontColor1), .fontSize = gui.io.fontSize }));
}

void text_label_centered(GUIManager& gui, std::string_view val) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
        }
    }) {
        CLAY_TEXT(gui.strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(gui.io.theme->frontColor1), .fontSize = gui.io.fontSize, .textAlignment = CLAY_TEXT_ALIGN_CENTER}));
    }
}

void text_label_light_centered(GUIManager& gui, std::string_view val) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
        }
    }) {
        CLAY_TEXT(gui.strArena.std_str_to_clay_str(val), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(gui.io.theme->frontColor2), .fontSize = gui.io.fontSize, .textAlignment = CLAY_TEXT_ALIGN_CENTER}));
    }
}

void mutable_text_label(GUIManager& gui, const char* id, const std::string& val) {
    Clay_TextElementConfig textConfig = CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(gui.io.theme->frontColor1), .fontSize = gui.io.fontSize });
    gui.element<MutableTextLabel>(id, val, textConfig);
}

}}
