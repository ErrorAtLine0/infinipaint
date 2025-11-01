#pragma once
#include "Element.hpp"

namespace GUIStuff {

class SelectableButton : public Element {
    public:
        enum class DrawType {
            TRANSPARENT,
            FILLED,
            FILLED_INVERSE,
            TRANSPARENT_BORDER
        };

        void update(UpdateInputData& io, DrawType drawType, const std::function<void(SelectionHelper&, bool)>& elemUpdate, bool isSelected);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override;
        SelectionHelper selection;
};

}
