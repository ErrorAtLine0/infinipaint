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

#include "SaturationValueSquare.hpp"
#include "../GUIManager.hpp"
#include "ColorPicker.hpp"

namespace GUIStuff {

SaturationValueSquare::SaturationValueSquare(GUIManager& gui): Element(gui) {}

void SaturationValueSquare::layout(const Clay_ElementId& id, const SaturationValueSquareData& opts) {
    o = opts;
    update();

    CLAY(id, {
        .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}},
        .custom = {.customData = this}
    }) {
    }
}

void SaturationValueSquare::update() {
    if(currentHue != *o.hue || currentSaturation != *o.saturation || currentValue != *o.value) {
        currentHue = *o.hue;
        currentSaturation = *o.saturation;
        currentValue = *o.value;
        gui.invalidate_draw_element(this);
    }
}

void SaturationValueSquare::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(mouseHovering && button.down && button.button == InputManager::MouseButton::LEFT) {
        if(o.onHold) o.onHold();
        update_sv(button.pos);
        if(o.onChange) o.onChange();
        isHeld = true;
    }
    else if(isHeld && !button.down && button.button == InputManager::MouseButton::LEFT) {
        if(o.onRelease) o.onRelease();
        isHeld = false;
    }
}

void SaturationValueSquare::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(isHeld) {
        update_sv(motion.pos);
        if(o.onChange) o.onChange();
    }
}

void SaturationValueSquare::update_sv(Vector2f p) {
    *o.saturation = std::clamp((p.x() - boundingBox.value().min.x()) / boundingBox.value().width(), 0.0f, 1.0f);
    *o.value = 1.0f - std::clamp((p.y() - boundingBox.value().min.y()) / boundingBox.value().height(), 0.0f, 1.0f);
}

void SaturationValueSquare::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    auto& bb = boundingBox.value();

    canvas->save();
    canvas->translate(bb.min.x(), bb.min.y());
    canvas->scale(bb.width(), bb.height());
    canvas->clipRect(SkRect::MakeXYWH(0.0f, 0.0f, 1.0f, 1.0f), skiaAA);

    SkPaint svSelectionAreaPaint;
    svSelectionAreaPaint.setShader(ColorPickerShaders::get_sv_selection_shader(currentHue / 360.0f));
    canvas->drawPaint(svSelectionAreaPaint);
    canvas->restore();

    canvas->save();
    canvas->translate(bb.min.x(), bb.min.y());

    SkPaint selectionLinePaint({1.0f, 1.0f, 1.0f, 1.0f});
    selectionLinePaint.setAntiAlias(skiaAA);
    selectionLinePaint.setStrokeWidth(1.0f);

    float valueY = bb.height() * (1.0f - currentValue);
    canvas->drawLine(0.0f, valueY, bb.width(), valueY, selectionLinePaint);
    float saturationX = bb.width() * currentSaturation;
    canvas->drawLine(saturationX, 0.0f, saturationX, bb.height(), selectionLinePaint);

    canvas->restore();
}

}
