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

#include "AlphaSlider.hpp"
#include "../GUIManager.hpp"
#include "ColorPicker.hpp"

namespace GUIStuff {

AlphaSlider::AlphaSlider(GUIManager& gui): Element(gui) {}

void AlphaSlider::layout(const Clay_ElementId& id, const AlphaSliderData& opts) {
    o = opts;
    update();

    CLAY(id, {
        .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}},
        .custom = {.customData = this}
    }) {
    }
}

void AlphaSlider::update() {
    if(currentG != *o.g || currentB != *o.b || currentR != *o.r || currentAlpha != *o.alpha) {
        currentR = *o.r;
        currentG = *o.g;
        currentB = *o.b;
        currentAlpha = *o.alpha;
        gui.invalidate_draw_element(this);
    }
}

void AlphaSlider::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(mouseHovering && button.down && button.button == InputManager::MouseButton::LEFT) {
        if(o.onHold) o.onHold();
        update_alpha(button.pos);
        if(o.onChange) o.onChange();
        isHeld = true;
    }
    else if(isHeld && !button.down && button.button == InputManager::MouseButton::LEFT) {
        if(o.onRelease) o.onRelease();
        isHeld = false;
    }
}

void AlphaSlider::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(isHeld) {
        update_alpha(motion.pos);
        if(o.onChange) o.onChange();
    }
}

void AlphaSlider::update_alpha(Vector2f p) {
    *o.alpha = std::clamp((p.x() - boundingBox.value().min.x()) / boundingBox.value().width(), 0.0f, 1.0f);
}

void AlphaSlider::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    auto& bb = boundingBox.value();

    canvas->save();
    canvas->translate(bb.min.x(), bb.min.y());
    canvas->clipRect(SkRect::MakeXYWH(0.0f, 0.0f, bb.width(), bb.height()), skiaAA);

    SkPaint alphaBarPaint;
    alphaBarPaint.setShader(ColorPickerShaders::get_alpha_bar_shader({currentR, currentG, currentB}, bb.width()));
    canvas->drawPaint(alphaBarPaint);

    SkPaint selectionLinePaint({1.0f, 1.0f, 1.0f, 1.0f});
    selectionLinePaint.setAntiAlias(skiaAA);
    selectionLinePaint.setStrokeWidth(1.0f);
    canvas->drawLine(currentAlpha * bb.width(), 0.0f, currentAlpha * bb.width(), bb.height(), selectionLinePaint);

    canvas->restore();
}

}
