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
#include "../Elements/LayoutElement.hpp"

namespace GUIStuff { namespace ElementHelpers {

void top_to_bottom_window_popup_layout(GUIManager& gui, const char* id, Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void(LayoutElement*)>& innerContent, const LayoutElement::Callbacks& callbacks);
void left_to_right_layout(GUIManager& gui, Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& innerContent);
void left_to_right_line_layout(GUIManager& gui, const std::function<void()>& innerContent);
void left_to_right_line_centered_layout(GUIManager& gui, const std::function<void()>& innerContent);

struct WindowFillSideBarConfig {
    enum class Direction {
        TOP,
        BOTTOM,
        LEFT,
        RIGHT
    } dir = Direction::TOP;
    SkColor4f backgroundColor = {0.0f, 0.0f, 0.0f, 0.0f};
    Clay_BorderElementConfig border;
};

void window_fill_side_bar(GUIManager& gui, const WindowFillSideBarConfig& config, const std::function<void()>& innerContent);
void window_gap_side_bar(GUIManager& gui, const WindowFillSideBarConfig::Direction& dir);

struct DropDownPopupLayout {
    float popupOffset = 4.0f;
    Element* button = nullptr;
    std::function<void()> clickAwayCallback;
    std::function<void()> clickUpAwayCallback;
    Vector2f entrySize;
    size_t entryCount;
    std::function<void(size_t i)> entryLayout;
};

struct AttachToButtonPopupLayout {
    float popupOffset = 4.0f;
    Element* button = nullptr;
    std::function<void()> clickAwayCallback;
    std::function<void()> clickUpAwayCallback;
    Vector2f popupSize;
    std::function<void()> innerLayout;
};

void attach_to_button_popup_layout(GUIManager& gui, const char* id, AttachToButtonPopupLayout d);
void dropdown_many_element_popup_layout(GUIManager& gui, const char* id, DropDownPopupLayout d);

}}
