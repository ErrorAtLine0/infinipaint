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

#include "RadioButtonHelpers.hpp"
#include "../Elements/RadioButton.hpp"
#include "LayoutHelpers.hpp"
#include "TextLabelHelpers.hpp"

namespace GUIStuff { namespace ElementHelpers {

void radio_button_field(GUIManager& gui, const char* id, std::string_view name, const std::function<bool()>& isTicked, const std::function<void()>& onClick) {
    left_to_right_line_layout(gui, [&]() {
        gui.element<RadioButton>(id, isTicked, onClick);
        text_label(gui, name);
    });
}

}}
