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
#include "../GUIManager.hpp"
#include "TextBoxHelpers.hpp"
#include "../Elements/NumberSlider.hpp"

namespace GUIStuff { namespace ElementHelpers {
    template <typename T> void slider_scalar_field(GUIManager& gui, const char* id, std::string_view name, T* val, T min, T max, TextBoxScalarOptions options = {}) {
        gui.new_id(id, [&] {
            left_to_right_line_layout(gui, [&]() {
                text_label(gui, name);
                input_scalar(gui, "textbox", val, min, max, options);
            });
            gui.element<NumberSlider<T>>("slider", val, min, max, NumberSliderData{
                .onChange = options.onEdit,
                .onHold = options.onSelect,
                .onRelease = options.onDeselect
            });
        });
    }
}}
