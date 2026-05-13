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

#include "RadioButton.hpp"
#include "../GUIManager.hpp"
#include "../../TimePoint.hpp"

namespace GUIStuff {

RadioButton::RadioButton(GUIManager& gui):
    Element(gui) {}

void RadioButton::layout(const Clay_ElementId& id, const std::function<bool()>& isTicked, const std::function<void()>& onClick) {
    this->onClick = onClick;
    this->isTicked = isTicked;

    float size = 20;
    CLAY(id, {
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(size), .height = CLAY_SIZING_FIXED(size)}
        },
        .custom = { .customData = this }
    }) {}
}

void RadioButton::update() {
    if(smooth_two_way_animation_time_check_for_change(hoverAnimation, gui.io.deltaTime, is_hovering_animation(), RADIOBUTTON_ANIMATION_TIME))
        gui.invalidate_draw_element(this);
    if(oldIsTicked != isTicked()) {
        gui.invalidate_draw_element(this);
        oldIsTicked = isTicked();
    }
}

bool RadioButton::is_hovering_animation() {
    return mouseHovering && (!gui.last_interaction_is_touch() || isHeld);
}

void RadioButton::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down) {
        gui.set_post_callback_func([&] { if(onClick) onClick(); });
        isHeld = true;
    }
    else
        isHeld = false;
}

void RadioButton::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
    if(mouseHovering && touch.down) {
        gui.set_post_callback_func([&] { if(onClick) onClick(); });
        isHeld = true;
    }
    else
        isHeld = false;
}

void RadioButton::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    auto& bb = boundingBox.value();

    canvas->save();
    canvas->translate(bb.min.x(), bb.min.y());
    canvas->scale(bb.width(), bb.height());
    canvas->translate(0.5f, 0.5f);

    if(isTicked()) {
        SkPaint p;
        p.setAntiAlias(skiaAA);
        p.setColor4f(convert_vec4<SkColor4f>(io.theme->fillColor1));
        p.setStyle(SkPaint::kFill_Style);
        canvas->drawCircle(0.0f, 0.0f, 0.5f, p);

        SkPaint innerCircleP;
        innerCircleP.setAntiAlias(skiaAA);
        innerCircleP.setColor4f(convert_vec4<SkColor4f>(io.theme->backColor2));
        innerCircleP.setStyle(SkPaint::kFill_Style);

        static BezierEasing easeRadius(0.68, -2.55, 0.265, 3.55);
        float lerpTime2 = easeRadius(hoverAnimation / RADIOBUTTON_ANIMATION_TIME);
        float innerCircleRadius = lerp_vec(0.3f, 0.2f, lerpTime2);

        canvas->drawCircle(0.0f, 0.0f, innerCircleRadius, innerCircleP);
    }
    else {
        SkPaint p;
        p.setAntiAlias(skiaAA);
        p.setColor4f(convert_vec4<SkColor4f>(is_hovering_animation() ? io.theme->fillColor1 : io.theme->backColor2));
        p.setStyle(SkPaint::kStroke_Style);
        p.setStrokeWidth(0.15f);
        canvas->drawCircle(0.0f, 0.0f, 0.5f, p);
    }

    canvas->restore();
}

}
