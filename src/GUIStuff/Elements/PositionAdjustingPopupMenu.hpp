#pragma once
#include "Element.hpp"
#include "LayoutElement.hpp"

namespace GUIStuff {

class PositionAdjustingPopupMenu : public Element {
    public:
        PositionAdjustingPopupMenu(GUIManager& gui);
        void layout(Vector2f popupPos, const std::function<void()>& innerContent);
    private:
        LayoutElement* layoutElement = nullptr;
};

}

