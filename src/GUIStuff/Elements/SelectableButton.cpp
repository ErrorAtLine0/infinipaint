#include "SelectableButton.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

SelectableButton::SelectableButton(GUIManager& gui):
    Element(gui) {}

void SelectableButton::layout(const Clay_ElementId& id, const Data& d) {
    auto& io = gui.io;

    SkColor4f borderColor;
    SkColor4f backgroundColorHighlight;
    SkColor4f backgroundColor;

    onClick = d.onClick;

    if(isHeld || ((d.isSelected || mouseHovering) && d.drawType == DrawType::TRANSPARENT_BORDER))
        borderColor = io.theme->fillColor1;
    else if(d.drawType == DrawType::TRANSPARENT_BORDER)
        borderColor = io.theme->backColor2;
    else
        borderColor = SkColor4f{0.0f, 0.0f, 0.0f, 0.0f};

    if(d.isSelected)
        backgroundColorHighlight = color_mul_alpha(io.theme->fillColor1, 0.4f);
    else if(mouseHovering || isHeld)
        backgroundColorHighlight = color_mul_alpha(io.theme->fillColor1, 0.2f);
    else
        backgroundColorHighlight = {0.0f, 0.0f, 0.0f, 0.0f};

    if(d.drawType == DrawType::TRANSPARENT_ALL || d.drawType == DrawType::TRANSPARENT_BORDER)
        backgroundColor = {0.0f, 0.0f, 0.0f, 0.0f};
    else if(d.drawType == DrawType::FILLED)
        backgroundColor = io.theme->backColor2;
    else
        backgroundColor = io.theme->fillColor2;

    CLAY(id, {.layout = { 
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
                .padding = (d.drawType == DrawType::TRANSPARENT_BORDER) ? CLAY_PADDING_ALL(0) : CLAY_PADDING_ALL(4),
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
            },
            .backgroundColor = convert_vec4<Clay_Color>(backgroundColorHighlight),
            .cornerRadius = CLAY_CORNER_RADIUS(4)
        }) {
            if(d.innerContent)
                d.innerContent({.isSelected = d.isSelected, .isHovering = mouseHovering, .isHeld = isHeld});
        }
    }
}

void SelectableButton::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    bool oldIsHeld = isHeld;
    isHeld = mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down;
    if(isHeld) {
        gui.set_post_callback_func([&]{ if(onClick) onClick(); });
        gui.set_to_layout();
    }
    else if(mouseHovering != oldIsHovering || isHeld != oldIsHeld)
        gui.set_to_layout();
    oldIsHovering = mouseHovering;
}

void SelectableButton::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(mouseHovering != oldIsHovering)
        gui.set_to_layout();
    oldIsHovering = mouseHovering;
}

}
