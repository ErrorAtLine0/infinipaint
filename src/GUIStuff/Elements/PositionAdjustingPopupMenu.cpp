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

#include "PositionAdjustingPopupMenu.hpp"
#include "../GUIManager.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff {

PositionAdjustingPopupMenu::PositionAdjustingPopupMenu(GUIManager& gui): Element(gui) {}

void PositionAdjustingPopupMenu::layout(const Clay_ElementId& id, Vector2f popupPos, const std::function<void()>& innerContent, const LayoutElement::Callbacks& callbacks) {
    if(layoutElement && layoutElement->get_bb().has_value()) {
        auto& bb = layoutElement->get_bb().value();
        if((popupPos.y() + bb.height()) > gui.io.windowSize.y())
            popupPos.y() -= bb.height();
        if((popupPos.x() + bb.width()) > gui.io.windowSize.x())
            popupPos.x() -= bb.width();
    }

    layoutElement = gui.element<LayoutElement>("popup", [&] (LayoutElement*, const Clay_ElementId& lId) {
        CLAY(lId, {
            .floating = {
                .offset = {popupPos.x(), popupPos.y()},
                .zIndex = gui.get_z_index(),
                .attachTo = CLAY_ATTACH_TO_ROOT
            }
        }) {
            innerContent();
        }
    }, callbacks);
}

}
