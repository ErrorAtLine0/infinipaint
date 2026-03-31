#include "CheckBox.hpp"
#include <include/core/SkPath.h>
#include <include/core/SkPathBuilder.h>
#include "../GUIManager.hpp"

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

bool CheckBox::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) {
    isHovering = mouseHovering;
    if(isHovering && button.button == InputManager::MouseButton::LEFT && button.down)
        gui.set_post_callback_func([&](){if(onClick) onClick();});
    return Element::input_mouse_button_callback(button, mouseHovering);
}

bool CheckBox::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) {
    isHovering = mouseHovering;
    return Element::input_mouse_motion_callback(motion, mouseHovering);
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
        p.setColor4f(convert_vec4<SkColor4f>(isHovering ? io.theme->fillColor1 : io.theme->fillColor2));
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

        float lerpTime2 = anim(smooth_two_way_time(hoverAnimation2, io.deltaTime, isHovering, CHECKBOX_ANIMATION_TIME));

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
