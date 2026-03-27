#pragma once
#include "Element.hpp"
#include "SelectableButton.hpp"
#include "SVGIcon.hpp"

namespace GUIStuff {

class MovableTabList : public Element {
    public:
        struct Data {
            std::vector<std::pair<std::string, std::string>> tabNames;
            size_t selectedTab = 0;
            std::function<void(size_t)> changeSelectedTab;
            std::function<void(size_t)> closeTab;
        };

        MovableTabList(GUIManager& gui);
        void layout(const Data& d);
};

}
