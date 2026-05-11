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

#include "CheckBoxHelpers.hpp"
#include "LayoutHelpers.hpp"
#include "TextLabelHelpers.hpp"
#include "../Elements/CheckBox.hpp"

namespace GUIStuff { namespace ElementHelpers {

void checkbox_boolean(GUIManager& gui, const char* id, bool* d, const std::function<void()>& onClick) {
    gui.element<CheckBox>(id, [d]{ return *d; }, [d, onClick] {
        *d = !(*d);
        if(onClick)
            onClick();
    });
}

void checkbox_field(GUIManager& gui, const char* id, std::string_view name, const std::function<bool()>& isTicked, const std::function<void()>& onClick) {
    left_to_right_line_layout(gui, [&]() {
        gui.element<CheckBox>(id, isTicked, onClick);
        text_label(gui, name);
    });
}

void checkbox_boolean_field(GUIManager& gui, const char* id, std::string_view name, bool* d, const std::function<void()>& onClick) {
    left_to_right_line_layout(gui, [&]() {
        checkbox_boolean(gui, id, d, onClick);
        text_label(gui, name);
    });
}

}}
