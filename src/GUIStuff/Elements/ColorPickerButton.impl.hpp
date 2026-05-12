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

#include "ColorPickerButton.decl.hpp"
#include "../ElementHelpers/ColorPickerItemsHelpers.hpp"
#include "../ElementHelpers/ButtonHelpers.hpp"
#include "../ElementHelpers/LayoutHelpers.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff {

template <typename T> ColorPickerButton<T>::ColorPickerButton(GUIManager& gui): Element(gui) {}

template <typename T> void ColorPickerButton<T>::layout(const Clay_ElementId& id, T* val, const ColorPickerButtonData& d) {
    this->data = d;

    Element* button = GUIStuff::ElementHelpers::color_button(gui, "Color Picker Button", val, {
        .isSelected = isOpen,
        .hasAlpha = data.hasAlpha,
        .onClick = [&]{
            isOpen = !isOpen;
            gui.set_to_layout();
        }
    });
    if(isOpen) {
        gui.set_z_index(gui.get_z_index() + 1, [&] {
            ElementHelpers::attach_to_button_popup_layout(gui, "Color Picker", {
                .button = button,
                .clickAwayCallback = [&] { isOpen = false; },
                .popupSize = {300.0f, 300.0f},
                .innerLayout = [&] {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .padding = CLAY_PADDING_ALL(gui.io.theme->padding1),
                            .childGap = gui.io.theme->childGap1,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        },
                    }) {
                        ElementHelpers::color_picker_items(gui, "c", val, {
                            .forceAspectRatioOnColorPicker = false,
                            .hasAlpha = data.hasAlpha,
                            .onEdit = data.onEdit,
                            .onSelect = data.onSelect,
                            .onDeselect = data.onDeselect
                        });
                    }
                }
            });
        });
    }
}

}
