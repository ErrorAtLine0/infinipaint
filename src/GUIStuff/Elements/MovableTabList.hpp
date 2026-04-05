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
