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

#include "GridScrollArea.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIManager.hpp"
#include "ManyElementScrollArea.hpp"

namespace GUIStuff {

GridScrollArea::GridScrollArea(GUIManager& gui): Element(gui) {}

void GridScrollArea::layout(const Clay_ElementId& id, const Options& o) {
    options = o;

    if(boundingBox.has_value())
        rowWidth = boundingBox.value().width(); // Use grid element bounding box instead of inner content width because row width will decide whether a scroll bar appears or not, which will change the inner content width (circular dependency)

    ManyElementScrollArea::Options opts;
    opts.clipHorizontal = true;
    opts.entryHeight = options.entryHeight;
    opts.innerContentExtraCallback = [&, extraCallback = options.innerContentExtraCallback] (const ScrollArea::InnerContentParameters& contParams) {
        if(extraCallback) extraCallback(contParams);
    };
    size_t entriesPerRow = std::max<size_t>(rowWidth / options.entryWidth, 1);
    size_t rowCount = (options.entryCount + entriesPerRow - 1) / entriesPerRow;
    opts.entryCount = rowCount;
    opts.scrollbar = options.scrollbar;
    
    uint16_t xPadding = 0;
    uint16_t xChildGap = 0;
    float entryWidth = options.entryWidth;
    if(rowCount > 1 || (rowCount == 1 && ((options.entryCount + entriesPerRow) / entriesPerRow) > 1)) { // More than 1 row or 1st row is full
        float padding = std::max<float>(rowWidth - (entriesPerRow * options.entryWidth), 0) / (entriesPerRow + 1);
        xChildGap = xPadding = padding;
        if(o.entryWidthIsMinimum) {
            xPadding = padding / 2;
            entryWidth = padding + options.entryWidth;
            xChildGap = 0;
        }
        else
            xPadding = xChildGap = padding;
    }

    opts.elementContent = [&] (size_t rowIndex) {
        gui.new_id("grid row", [&] {
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                    .padding = {.left = xPadding, .right = xPadding},
                    .childGap = xChildGap,
                    .childAlignment = { .x = options.childAlignmentX}
                }
            }) {
                size_t startIndex = rowIndex * entriesPerRow;
                size_t endIndex = std::min(rowIndex * entriesPerRow + entriesPerRow, o.entryCount);
                for(size_t i = startIndex; i < endIndex; i++) {
                    gui.new_id(static_cast<int64_t>(i - startIndex), [&] {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_FIXED(entryWidth), .height = CLAY_SIZING_FIXED(options.entryHeight)},
                            }
                        }) {
                            options.elementContent(i);
                        }
                    });
                }
            }
        });
    };
    CLAY(id, {
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
        }
    }) {
        gui.element<ManyElementScrollArea>("scroll many area", opts);
    }
}

}
