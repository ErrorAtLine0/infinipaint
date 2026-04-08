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
