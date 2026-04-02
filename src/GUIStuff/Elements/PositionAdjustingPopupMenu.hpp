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

