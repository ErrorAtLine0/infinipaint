#pragma once
#include "Element.hpp"
#include "SelectableButton.hpp"
#include "SVGIcon.hpp"

namespace GUIStuff {

class MovableTabList : public Element {
    public:
        void update(UpdateInputData& io, const std::vector<std::pair<std::string, std::string>>& tabNames, size_t& selectedTab, std::optional<size_t>& closedTab, const std::function<void()>& elemUpdate);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override;
        SelectionHelper selection;
    private:
        std::vector<std::pair<std::string, std::string>> oldTabNames;
        std::vector<SelectableButton> buttons;
        std::vector<SelectableButton> closeButtons;
        std::vector<SVGIcon> closeIcons;
        std::vector<SVGIcon> tabIcons;
};

}
