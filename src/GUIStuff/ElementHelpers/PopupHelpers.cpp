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

#include "PopupHelpers.hpp"

namespace GUIStuff { namespace ElementHelpers {

void paint_circle_popup_menu(GUIManager& gui, const char* id, const Vector2f& centerPos, const PaintCircleMenu::Data& val) {
    constexpr float size = 300.0f;
    gui.new_id(id, [&] {
        CLAY_AUTO_ID({.layout = { 
                .sizing = {.width = CLAY_SIZING_FIXED(size), .height = CLAY_SIZING_FIXED(size)}
            },
            .floating = {
                .offset = {centerPos.x() - size * 0.5f, centerPos.y() - size * 0.5f},
                .zIndex = gui.get_z_index(),
                .attachTo = CLAY_ATTACH_TO_ROOT
            }
        }) {
            gui.element<PaintCircleMenu>("paint circle menu", val);
        }
    });
}

}}
