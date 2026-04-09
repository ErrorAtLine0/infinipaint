#include "RadioButton.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

RadioButton::RadioButton(GUIManager& gui):
    Element(gui) {}

void RadioButton::layout(const Clay_ElementId& id, const std::function<bool()>& isTicked, const std::function<void()>& onClick) {
    this->onClick = onClick;
    this->isTicked = isTicked;

    CLAY(id, {
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)}
        },
        .custom = { .customData = this }
    }) {}
}

void RadioButton::update() {
    if(smooth_two_way_animation_time_check_for_change(hoverAnimation, gui.io.deltaTime, mouseHovering, RADIOBUTTON_ANIMATION_TIME))
        gui.invalidate_draw_element(this);
    if(oldIsTicked != isTicked()) {
        gui.invalidate_draw_element(this);
        oldIsTicked = isTicked();
    }
}

void RadioButton::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down)
        gui.set_post_callback_func([&] { if(onClick) onClick(); });
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
        p.setColor4f(convert_vec4<SkColor4f>(mouseHovering ? io.theme->fillColor1 : io.theme->backColor2));
        p.setStyle(SkPaint::kStroke_Style);
        p.setStrokeWidth(0.15f);
        canvas->drawCircle(0.0f, 0.0f, 0.5f, p);
    }

    canvas->restore();
}

}
