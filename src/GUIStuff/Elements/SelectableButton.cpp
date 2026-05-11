/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

    inDynamicArea = gui.is_dynamic_area();
    instantResponse = d.instantResponse;
    onClick = [&, this, d] {
        if(d.onClickButton)
            d.onClickButton(this);
        else if(d.onClick) d.onClick();
    };

    if(isHeld || ((d.isSelected || isHovering) && d.drawType == DrawType::TRANSPARENT_BORDER))
        borderColor = io.theme->fillColor1;
    else if(d.drawType == DrawType::TRANSPARENT_BORDER)
        borderColor = io.theme->backColor2;
    else
        borderColor = SkColor4f{0.0f, 0.0f, 0.0f, 0.0f};

    if(d.isSelected)
        backgroundColorHighlight = color_mul_alpha(io.theme->fillColor1, 0.4f);
    else if(isHovering || isHeld)
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
                d.innerContent({.isSelected = d.isSelected, .isHovering = isHovering, .isHeld = isHeld});
        }
    }
}

void SelectableButton::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    bool oldIsHeld = isHeld;
    isHeld = mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down;
    if(isHeld) {
        if(instantResponse)
            gui.set_post_callback_func(onClick);
        gui.set_to_layout();
    }
    else if(mouseHovering && oldIsHeld && button.button == InputManager::MouseButton::LEFT && !button.down) {
        if(!instantResponse)
            gui.set_post_callback_func(onClick);
        gui.set_to_layout();
    }
    else if(mouseHovering != isHovering || isHeld != oldIsHeld)
        gui.set_to_layout();
    isHovering = mouseHovering;
}

void SelectableButton::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(mouseHovering != isHovering)
        gui.set_to_layout();
    isHovering = mouseHovering;
}

void SelectableButton::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
    bool oldIsHeld = isHeld;
    bool oldIsHovering = isHovering;
    isHeld = mouseHovering && touch.down;
    isHovering = mouseHovering && touch.down;
    if(isHeld) {
        if(instantResponse)
            gui.set_post_callback_func(onClick);
        gui.set_to_layout();
    }
    else if(mouseHovering && oldIsHeld && !touch.down) {
        if(!instantResponse)
            gui.set_post_callback_func(onClick);
        gui.set_to_layout();
    }
    else if(oldIsHovering != isHovering || isHeld != oldIsHeld)
        gui.set_to_layout();
}

void SelectableButton::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {
    if((isHovering || isHeld) && (inDynamicArea || !mouseHovering)) {
        isHovering = false;
        isHeld = false;
        gui.set_to_layout();
    }
}

}
