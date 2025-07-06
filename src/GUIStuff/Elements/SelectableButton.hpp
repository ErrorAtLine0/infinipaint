#pragma once
#include "Element.hpp"

namespace GUIStuff {

class SelectableButton : public Element {
    public:
        void update(UpdateInputData& io, bool hasBorders, bool hasBorderPadding, const std::function<void(SelectionHelper&, bool)>& elemUpdate, bool isSelected);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override;
        SelectionHelper selection;
};

}
