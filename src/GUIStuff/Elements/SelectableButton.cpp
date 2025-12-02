#include "SelectableButton.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff {

void SelectableButton::update(UpdateInputData& io, DrawType drawType, const std::function<void(SelectionHelper&, bool)>& elemUpdate, bool isSelected) {
    SkColor4f borderColor;
    SkColor4f backgroundColorHighlight;
    SkColor4f backgroundColor;

    if(selection.held || ((isSelected || selection.hovered) && drawType == DrawType::TRANSPARENT_BORDER))
        borderColor = io.theme->fillColor1;
    else if(drawType == DrawType::TRANSPARENT_BORDER)
        borderColor = io.theme->backColor2;
    else
        borderColor = SkColor4f{0.0f, 0.0f, 0.0f, 0.0f};

    if(isSelected)
        backgroundColorHighlight = color_mul_alpha(io.theme->fillColor1, 0.4f);
    else if(selection.hovered || selection.held)
        backgroundColorHighlight = color_mul_alpha(io.theme->fillColor1, 0.2f);
    else
        backgroundColorHighlight = {0.0f, 0.0f, 0.0f, 0.0f};

    if(drawType == DrawType::TRANSPARENT_ALL || drawType == DrawType::TRANSPARENT_BORDER)
        backgroundColor = {0.0f, 0.0f, 0.0f, 0.0f};
    else if(drawType == DrawType::FILLED)
        backgroundColor = io.theme->backColor2;
    else
        backgroundColor = io.theme->fillColor2;

    CLAY_AUTO_ID({.layout = { 
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
            .childGap = 0,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER }
        },
        .backgroundColor = convert_vec4<Clay_Color>(backgroundColor),
        .cornerRadius = CLAY_CORNER_RADIUS(4),
        .border = {
            .color = convert_vec4<Clay_Color>(borderColor),
            .width = CLAY_BORDER_OUTSIDE(2)
        }
    }) {
        CLAY_AUTO_ID({.layout = { 
                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                .padding = (drawType == DrawType::TRANSPARENT_BORDER) ? CLAY_PADDING_ALL(0) : CLAY_PADDING_ALL(4),
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = convert_vec4<Clay_Color>(backgroundColorHighlight),
            .cornerRadius = CLAY_CORNER_RADIUS(4)
        }) {
            selection.update(Clay_Hovered(), io.mouse.leftClick, io.mouse.leftHeld);
            elemUpdate(selection, isSelected);
        }
    }
}

void SelectableButton::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
}

}
