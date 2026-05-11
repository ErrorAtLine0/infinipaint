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
#include "SelectableButton.hpp"
#include "SVGIcon.hpp"

namespace GUIStuff {

struct MovableTabListData {
    struct IconNamePair {
        std::string iconPath;
        std::string name;
    };
    std::vector<IconNamePair> tabNames;
    size_t selectedTab = 0;
    std::function<void(size_t)> changeSelectedTab;
    std::function<void(size_t)> closeTab;
};

class MovableTabList : public Element {
    public:
        MovableTabList(GUIManager& gui);
        void layout(const Clay_ElementId& id, const MovableTabListData& newData = {});
    private:
        MovableTabListData d;
};

}
