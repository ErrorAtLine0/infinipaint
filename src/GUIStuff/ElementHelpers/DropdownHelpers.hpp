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
#include "../Elements/DropDown.hpp"

namespace GUIStuff { namespace ElementHelpers {

template <typename T> void number_dropdown(GUIManager& gui, const char* id, T* data, int32_t minData, int32_t maxData, int32_t step, const std::function<void()>& onChange = nullptr) {
    std::vector<std::string> dropdownStrings;
    size_t closestSelectionDistance = std::numeric_limits<size_t>::max();
    std::shared_ptr<size_t> currentSelection = std::make_shared<size_t>(0);
    {
        size_t i = 0;
        for(int32_t newSelection = minData; newSelection <= maxData; newSelection += step) {
            size_t newDistance = std::abs(static_cast<int32_t>(*data) - newSelection);
            if(newDistance < closestSelectionDistance) {
                closestSelectionDistance = newDistance;
                *currentSelection = i;
            }
            dropdownStrings.emplace_back(std::to_string(newSelection));
            i++;
        }
    }

    gui.element<DropDown<size_t>>(id, currentSelection.get(), dropdownStrings, DropdownOptions{
        .onClick = [minData, step, data, onChange, currentSelection] {
            int32_t newVal = minData;
            for(size_t i = 0; i < *currentSelection; i++)
                newVal += step;
            *data = static_cast<T>(newVal);
            if(onChange) onChange();
        }
    });
}

}}
