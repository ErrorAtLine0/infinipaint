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

#include "CheckBox.hpp"
#include <include/core/SkPath.h>
#include <include/core/SkPathBuilder.h>
#include "../GUIManager.hpp"
#include "../../TimePoint.hpp"

namespace GUIStuff {

CheckBox::CheckBox(GUIManager& gui):
    Element(gui) {}

void CheckBox::layout(const Clay_ElementId& id, const std::function<bool()>& isTicked, const std::function<void()>& onClick) {
    this->isTicked = isTicked;
    this->onClick = onClick;

    CLAY(id, {
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(15), .height = CLAY_SIZING_FIXED(15)}
        },
        .custom = { .customData = this }
    }) {}
}

void CheckBox::update() {
    if(smooth_two_way_animation_time_check_for_change(hoverAnimation, gui.io.deltaTime, is_hovering_animation(), CHECKBOX_ANIMATION_TIME))
        gui.invalidate_draw_element(this);
    if(oldIsTicked != isTicked()) {
        gui.invalidate_draw_element(this);
        oldIsTicked = isTicked();
    }
}

bool CheckBox::is_hovering_animation() {
    return mouseHovering && (!gui.last_interaction_is_touch() || isHeld);
}

void CheckBox::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down) {
        gui.set_post_callback_func([&](){if(onClick) onClick();});
        isHeld = true;
    }
    else
        isHeld = false;
}

void CheckBox::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
    if(mouseHovering && touch.down) {
        gui.set_post_callback_func([&](){if(onClick) onClick();});
        isHeld = true;
    }
    else
        isHeld = false;
}

void CheckBox::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    auto& bb = boundingBox.value();

    canvas->save();
    canvas->translate(bb.min.x(), bb.min.y());
    canvas->scale(bb.width(), bb.height());
    canvas->translate(0.5f, 0.5f);

    SkPaint p;
    p.setAntiAlias(skiaAA);
    if(isTicked()) {
        SkRect checkBox = SkRect::MakeLTRB(-0.5f, -0.5f, 0.5f, 0.5f);
        p.setColor4f(convert_vec4<SkColor4f>(io.theme->fillColor1));
        p.setStyle(SkPaint::kFill_Style);
        canvas->drawRoundRect(checkBox, 0.25f, 0.25f, p);
    }
    else {
        SkRect checkBox = SkRect::MakeLTRB(-0.45f, -0.45f, 0.45f, 0.45f);
        p.setColor4f(convert_vec4<SkColor4f>(is_hovering_animation() ? io.theme->fillColor1 : io.theme->fillColor2));
        p.setStyle(SkPaint::kStroke_Style);
        p.setStrokeWidth(0.15f);
        canvas->drawRoundRect(checkBox, 0.25f, 0.25f, p);
    }

    if(isTicked()) {
        SkPaint checkP;
        checkP.setAntiAlias(skiaAA);
        checkP.setColor4f(io.theme->backColor1);
        checkP.setStyle(SkPaint::kFill_Style);
        checkP.setStrokeWidth(0.12);
        checkP.setStrokeCap(SkPaint::kRound_Cap);
        checkP.setStrokeJoin(SkPaint::kRound_Join);

        SkPathBuilder checkPathB;
        Vector2f checkP1{(4.5/17.0) - 0.5, (8.5/17.0) - 0.5};
        Vector2f checkP2{(7.5/17.0) - 0.5, (12.0/17.0) - 0.5};
        Vector2f checkP3{(12.5/17.0) - 0.5, (6.0/17.0) - 0.5};

        Vector2f rectP1{-0.2f, -0.2f};
        Vector2f rectP2{-0.2f,  0.2f};
        Vector2f rectP3{ 0.2f,  0.2f};
        Vector2f rectP4{ 0.2f, -0.2f};

        static BezierEasing anim{0.445, -0.733, 0.575, 1.627};

        float lerpTime2 = anim(hoverAnimation / CHECKBOX_ANIMATION_TIME);

        std::array<Vector2f, 5> points;

        points[0] = lerp_vec(checkP1, rectP1, lerpTime2);
        points[1] = lerp_vec(checkP2, rectP2, lerpTime2);
        points[2] = lerp_vec(checkP3, rectP3, lerpTime2);
        points[3] = lerp_vec(checkP2, rectP4, lerpTime2);
        points[4] = points[0];

        checkPathB.moveTo(points[0].x(), points[0].y());
        for(unsigned i = 1; i < 5; i++)
            checkPathB.lineTo(points[i].x(), points[i].y());
        checkPathB.close();

        SkPath checkPath = checkPathB.detach();
        canvas->drawPath(checkPath, checkP);
        checkP.setStyle(SkPaint::kStroke_Style);
        canvas->drawPath(checkPath, checkP);
    }
    canvas->restore();
}

}
