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
#include "Helpers/ConvertVec.hpp"
#include "ScrollArea.hpp"

namespace GUIStuff {

class ManyElementScrollArea : public Element {
    public:
        ManyElementScrollArea(GUIManager& gui);

        struct Options {
            float entryHeight;
            size_t entryCount;
            bool clipHorizontal = false;
            Clay_LayoutAlignmentX xAlign = CLAY_ALIGN_X_LEFT;
            Clay_SizingAxis xElementSize = CLAY_SIZING_GROW(0);
            ScrollArea::ScrollbarType scrollbar = ScrollArea::ScrollbarType::NORMAL;
            std::function<void(size_t elementIndex)> elementContent;
            std::function<void(const ScrollArea::InnerContentParameters&)> innerContentExtraCallback;
        };

        void layout(const Clay_ElementId& id, const Options& opts);

        ScrollArea* scrollArea = nullptr;
    private:
        Options options;
};

}
