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
#include "Helpers/ConvertVec.hpp"
#include "TextBoxHelpers.hpp"
#include "../Elements/ColorPicker.hpp"

namespace GUIStuff { namespace ElementHelpers {

struct ColorPickerItemsOptions {
    bool forceAspectRatioOnColorPicker = true;
    bool hasAlpha = true;
    std::function<void()> onEdit;
    std::function<void()> onSelect;
    std::function<void()> onDeselect;
};

template <typename T> void color_picker_items(GUIManager& gui, const char* id, T* val, const ColorPickerItemsOptions& options = {}) {
    gui.new_id(id, [&] {
        auto fullOnEdit = [&gui, oE = options.onEdit]() {
            if(oE) oE();
            gui.set_to_layout();
        };
        CLAY_AUTO_ID({
            .layout = {
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
            },
            .aspectRatio = {options.forceAspectRatioOnColorPicker ? 1.0f : 0.0f}
        }) {
            gui.element<ColorPicker<T>>("c", val, options.hasAlpha, ColorPickerData{ .onChange = fullOnEdit, .onHold = options.onSelect, .onRelease = options.onDeselect });
        }
        if(!gui.io.isTouchDevice) {
            left_to_right_line_layout(gui, [&]() {
                text_label(gui, "R");
                input_color_component_255(gui, "r", &(*val)[0], { .onEdit = fullOnEdit, .onSelect = options.onSelect, .onDeselect = options.onDeselect });
                text_label(gui, "G");
                input_color_component_255(gui, "g", &(*val)[1], { .onEdit = fullOnEdit, .onSelect = options.onSelect, .onDeselect = options.onDeselect });
                text_label(gui, "B");
                input_color_component_255(gui, "b", &(*val)[2], { .onEdit = fullOnEdit, .onSelect = options.onSelect, .onDeselect = options.onDeselect });
                if(options.hasAlpha) {
                    text_label(gui, "A");
                    input_color_component_255(gui, "a", &(*val)[3], { .onEdit = fullOnEdit, .onSelect = options.onSelect, .onDeselect = options.onDeselect });
                }
            });
            left_to_right_line_layout(gui, [&]() {
                text_label(gui, "Hex");
                input_color_hex(gui, "h", val, { .hasAlpha = options.hasAlpha, .onEdit = fullOnEdit, .onSelect = options.onSelect, .onDeselect = options.onDeselect });
            });
        }
    });
}

} }
