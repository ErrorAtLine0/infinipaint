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

namespace GUIStuff {

template <typename T> ColorPickerButton<T>::ColorPickerButton(GUIManager& gui): Element(gui) {}

template <typename T> void ColorPickerButton<T>::layout(const Clay_ElementId& id, T* val, const ColorPickerButtonData& d) {
    this->data = d;

    GUIStuff::ElementHelpers::color_button(gui, "Color Picker Button", val, {
        .isSelected = isOpen,
        .hasAlpha = data.hasAlpha,
        .onClick = [&]{
            isOpen = !isOpen;
            gui.set_to_layout();
        }
    });
    if(isOpen) {
        gui.set_z_index(gui.get_z_index() + 1, [&] {
            ElementHelpers::top_to_bottom_window_popup_layout(gui, "Color Picker", CLAY_SIZING_FIT(300), CLAY_SIZING_FIT(0), [&, val, data = data](LayoutElement*) {
                ElementHelpers::color_picker_items(gui, "c", val, {
                    .hasAlpha = data.hasAlpha,
                    .onEdit = data.onEdit,
                    .onSelect = data.onSelect,
                    .onDeselect = data.onDeselect
                });
            }, {
                .mouseButton = [&](LayoutElement* l, const InputManager::MouseButtonCallbackArgs& button) {
                    if(!l->mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down) {
                        isOpen = false;
                        gui.set_to_layout();
                    }
                }
            });
        });
    }
}

}
