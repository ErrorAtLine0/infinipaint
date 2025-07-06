#include "SelectableButton.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff {

void SelectableButton::update(UpdateInputData& io, bool hasBorders, bool hasBorderPadding, const std::function<void(SelectionHelper&, bool)>& elemUpdate, bool isSelected) {
    if(hasBorders) {
        SkColor4f borderColor;
        if(selection.held)
            borderColor = io.theme->fillColor1;
        else if(isSelected)
            borderColor = io.theme->fillColor2;
        else if(selection.hovered)
            borderColor = io.theme->fillColor2;
        else
            borderColor = io.theme->backColor3;

        SkColor4f backgroundColor;
        if(selection.held)
            backgroundColor = io.theme->backColor3;
        else if(isSelected)
            backgroundColor = io.theme->backColor2;
        else if(selection.hovered)
            backgroundColor = io.theme->backColor2;
        else
            backgroundColor = io.theme->backColor1;

        CLAY({.layout = { 
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                .padding = hasBorderPadding ? CLAY_PADDING_ALL(4) : CLAY_PADDING_ALL(0),
                .childGap = 4,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = convert_vec4<Clay_Color>(backgroundColor),
            .cornerRadius = CLAY_CORNER_RADIUS(4),
            .border = {
                .color = convert_vec4<Clay_Color>(borderColor),
                .width = CLAY_BORDER_OUTSIDE(4)
            }
        }) {
            selection.update(Clay_Hovered(), io.mouse.leftClick, io.mouse.leftHeld);
            elemUpdate(selection, isSelected);
        }
    }
    else {
        selection.update(Clay_Hovered(), io.mouse.leftClick, io.mouse.leftHeld);
        elemUpdate(selection, isSelected);
    }
}

void SelectableButton::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
}

}
