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

namespace GUIStuff { namespace ElementHelpers {

void radio_button_field(GUIManager& gui, const char* id, std::string_view name, const std::function<bool()>& isTicked, const std::function<void()>& onClick);

template <typename T> void radio_button_selector(GUIManager& gui, const char* id, T* data, const std::vector<std::pair<std::string_view, T>>& options, const std::function<void()>& onClick = nullptr) {
    gui.new_id(id, [&] {
        for(size_t i = 0; i < options.size(); i++) {
            gui.new_id(i, [&] {
                auto& option = options[i];
                radio_button_field(gui, "Option", option.first, [data, optionVal = option.second]{ return *data == optionVal; }, [data, optionVal = option.second, onClick]{ *data = optionVal; if(onClick) onClick(); });
            });
        }
    });
}

}}
