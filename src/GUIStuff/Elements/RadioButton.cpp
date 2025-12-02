#include "RadioButton.hpp"

namespace GUIStuff {

void RadioButton::update(UpdateInputData& io, bool newIsTicked, const std::function<void()>& elemUpdate) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)}
        },
        .custom = { .customData = this }
    }) {
        selection.update(Clay_Hovered(), io.mouse.leftClick, io.mouse.leftHeld);
        if(isTicked != newIsTicked) {
            hoverAnimation2 = RADIOBUTTON_ANIMATION_TIME;
            isTicked = newIsTicked;
        }
        if(elemUpdate)
            elemUpdate();
    }
}

void RadioButton::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
    auto bb = get_bb(command);

    canvas->save();
    canvas->translate(bb.min.x(), bb.min.y());
    canvas->scale(bb.width(), bb.height());
    canvas->translate(0.5f, 0.5f);

    if(isTicked) {
        SkPaint p;
        p.setColor4f(convert_vec4<SkColor4f>(io.theme->fillColor1));
        p.setStyle(SkPaint::kFill_Style);
        canvas->drawCircle(0.0f, 0.0f, 0.5f, p);

        SkPaint innerCircleP;
        innerCircleP.setColor4f(convert_vec4<SkColor4f>(io.theme->backColor2));
        innerCircleP.setStyle(SkPaint::kFill_Style);

        static BezierEasing easeRadius(0.68, -2.55, 0.265, 3.55);
        float lerpTime2 = easeRadius(smooth_two_way_time(hoverAnimation2, io.deltaTime, selection.hovered, RADIOBUTTON_ANIMATION_TIME));
        float innerCircleRadius = lerp_vec(0.3f, 0.2f, lerpTime2);

        canvas->drawCircle(0.0f, 0.0f, innerCircleRadius, innerCircleP);
    }
    else {
        SkPaint p;
        p.setColor4f(convert_vec4<SkColor4f>(selection.hovered ? io.theme->fillColor1 : io.theme->backColor2));
        p.setStyle(SkPaint::kStroke_Style);
        p.setStrokeWidth(0.15f);
        canvas->drawCircle(0.0f, 0.0f, 0.5f, p);
    }

    canvas->restore();
}

}
