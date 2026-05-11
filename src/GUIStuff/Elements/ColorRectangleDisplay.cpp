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

#include "ColorRectangleDisplay.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

sk_sp<SkRuntimeEffect> ColorRectangleDisplay::alphaBackgroundEffect;

ColorRectangleDisplay::ColorRectangleDisplay(GUIManager& gui): Element(gui) {}

void ColorRectangleDisplay::layout(const Clay_ElementId& id, const std::function<SkColor4f()>& colorFunc) {
    this->colorFunc = colorFunc;
    CLAY(id, {
        .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}},
        .custom = {this}
    }) {}
}

void ColorRectangleDisplay::update() {
    SkColor4f newDrawVal = colorFunc();
    if(newDrawVal != drawVal) {
        drawVal = newDrawVal;
        gui.invalidate_draw_element(this);
    }
}

void ColorRectangleDisplay::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    auto& bb = boundingBox.value();

    SkRect r = SkRect::MakeLTRB(bb.min.x(), bb.min.y(), bb.max.x(), bb.max.y());

    if(drawVal.fA == 1.0f) {
        SkPaint p;
        p.setColor4f(drawVal);
        p.setAntiAlias(skiaAA);
        canvas->drawRect(r, p);
    }
    else {
        canvas->save();
        canvas->clipRect(r, skiaAA);
        SkPaint alphaPaint;
        alphaPaint.setShader(get_alpha_background_shader());
        canvas->drawPaint(alphaPaint);
        canvas->drawPaint(SkPaint{SkColor4f(drawVal.fR, drawVal.fG, drawVal.fB, std::sqrt(drawVal.fA))}); // Square root for alpha to keep color visible
        canvas->restore();
    }
}

sk_sp<SkShader> ColorRectangleDisplay::get_alpha_background_shader() {
    if(!alphaBackgroundEffect) {
        auto [effect, err] = SkRuntimeEffect::MakeForShader(SkString(alphaBackgroundSksl));
        if(!err.isEmpty()) {
            std::cout << "[ColorRectangleDisplay::get_alpha_background_shader] Alpha background shader construction error\n";
            std::cout << err.c_str() << std::endl;
            return nullptr;
        }
        else
            alphaBackgroundEffect = effect;
    }
    SkRuntimeShaderBuilder builder(alphaBackgroundEffect);
    return builder.makeShader();
}

}
