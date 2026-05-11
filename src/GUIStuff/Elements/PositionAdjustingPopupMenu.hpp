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
#include "Element.hpp"
#include "LayoutElement.hpp"

namespace GUIStuff {

class PositionAdjustingPopupMenu : public Element {
    public:
        PositionAdjustingPopupMenu(GUIManager& gui);
        void layout(const Clay_ElementId& id, Vector2f popupPos, const std::function<void()>& innerContent, const LayoutElement::Callbacks& callbacks = {});
    private:
        LayoutElement* layoutElement = nullptr;
};

}

