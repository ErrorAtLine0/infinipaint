#include "RotateWheel.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

constexpr float WHEEL_WIDTH = 13.0f;

constexpr double ROTATE_BAR_SNAP_DISTANCE = 0.3;
constexpr double ROTATE_BAR_SNAP_DISTRIBUTION = std::numbers::pi * 0.25;

RotateWheel::RotateWheel(GUIManager& gui):
    Element(gui) {}

void RotateWheel::layout(double* rotateAngle, const std::function<void()>& onChange) {
    this->rotateAngle = rotateAngle;
    this->onChange = onChange;

    CLAY_AUTO_ID({
        .layout = { 
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
        },
        .custom = { .customData = this }
    }) {
    }
}

float RotateWheel::wheel_start() {
    return wheel_end() - WHEEL_WIDTH;
}

float RotateWheel::wheel_end() {
    return boundingBox.value().width() * 0.5f;
}

bool RotateWheel::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) {
    isHovering = mouseHovering;
    isHeld = isHovering && button.button == InputManager::MouseButton::LEFT && button.down;
    update_paint_circle_menu_mouse(button.pos, button.button == InputManager::MouseButton::LEFT && button.down);
    return Element::input_mouse_button_callback(button, mouseHovering);
}

bool RotateWheel::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) {
    update_paint_circle_menu_mouse(motion.pos, false);
    return Element::input_mouse_motion_callback(motion, mouseHovering);
}

void RotateWheel::update_paint_circle_menu_mouse(const Vector2f& p, bool leftClicked) {
    if(boundingBox.has_value()) {
        Vector2f vecFromCenter = (p - boundingBox.value().center()).normalized();
        float distFromCenter = vec_distance(p, boundingBox.value().center());
        isRotateBarHovered = isHovering && distFromCenter > wheel_start() && distFromCenter < wheel_end();
        isRotateBarHeld = isRotateBarHovered && isHeld;

        if(isRotateBarHeld) {
            gui.set_post_callback_func([&, vecFromCenter] {
                auto& rotationAngle = *rotateAngle;
                rotationAngle = std::atan2(vecFromCenter.y(), -vecFromCenter.x()) + std::numbers::pi;
                if(isRotateBarHovered) { // If we're hovering over the rotation bar, we should try snapping to specific angles
                    for(double snapPos = 0.0; snapPos < std::numbers::pi * 2.0 + 0.001; snapPos += ROTATE_BAR_SNAP_DISTRIBUTION) {
                        if(std::fabs(rotationAngle - snapPos) < ROTATE_BAR_SNAP_DISTANCE) {
                            rotationAngle = snapPos;
                            if(rotationAngle >= std::numbers::pi * 2.0 - 0.001) // Set 2pi back to 0
                                rotationAngle = 0;
                            break;
                        }
                    }
                }
            });
        }
    }
}

void RotateWheel::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    auto& bb = boundingBox.value();

    canvas->save();
    canvas->translate(bb.center().x(), bb.center().y());

    draw_rotate_wheel(canvas, io, skiaAA);

    canvas->restore();
}

void RotateWheel::draw_rotate_wheel(SkCanvas* canvas, UpdateInputData& io, bool skiaAA) {
    float wheelStart = wheel_start();
    float wheelEnd = wheel_end();

    float rotateBarMiddleRadius = (wheelEnd + wheelStart) / 2.0f;
    SkPaint rotateBarFill(io.theme->backColor2);
    rotateBarFill.setStroke(true);
    rotateBarFill.setStrokeWidth(wheelEnd - wheelStart);
    canvas->drawCircle(0.0f, 0.0f, rotateBarMiddleRadius, rotateBarFill);

    if(isRotateBarHovered) {
        for(double snapPos = 0.0; snapPos < std::numbers::pi * 2.0; snapPos += ROTATE_BAR_SNAP_DISTRIBUTION) {
            Vector2f lineDir{std::cos(snapPos), -std::sin(snapPos)};
            SkPaint p(snapPos == 0.0 ? io.theme->fillColor1 : io.theme->frontColor2);
            p.setAntiAlias(skiaAA);
            p.setStrokeWidth(1.0f);
            canvas->drawLine(lineDir.x() * wheelStart, lineDir.y() * wheelStart, lineDir.x() * wheelEnd, lineDir.y() * wheelEnd, p);
        }
    }

    SkPaint rotateBarHolderFill;
    rotateBarHolderFill.setAntiAlias(skiaAA);
    if(selection.held)
        rotateBarHolderFill.setColor4f(io.theme->fillColor1);
    else if(selection.hovered)
        rotateBarHolderFill.setColor4f(io.theme->fillColor2);
    else
        rotateBarHolderFill.setColor4f(io.theme->frontColor2);
    float rotateBarHolderRadius = (wheelEnd - wheelStart) / 2.0f;
    Vector2f rotateBarHolderPos{std::cos(*rotateAngle) * rotateBarMiddleRadius, -std::sin(*rotateAngle) * rotateBarMiddleRadius};
    canvas->drawCircle(rotateBarHolderPos.x(), rotateBarHolderPos.y(), rotateBarHolderRadius, rotateBarHolderFill);
    SkPaint rotateBarHolderOutline(io.theme->frontColor1);
    rotateBarHolderOutline.setAntiAlias(skiaAA);
    rotateBarHolderOutline.setStroke(true);
    rotateBarHolderOutline.setStrokeWidth(2.0f);
    canvas->drawCircle(rotateBarHolderPos.x(), rotateBarHolderPos.y(), rotateBarHolderRadius, rotateBarHolderOutline);

    SkPaint rotateBarOutline(io.theme->frontColor2);
    rotateBarOutline.setAntiAlias(skiaAA);
    rotateBarOutline.setStroke(true);
    rotateBarOutline.setStrokeWidth(1.0f);
    canvas->drawCircle(0.0f, 0.0f, wheelStart, rotateBarOutline);
    canvas->drawCircle(0.0f, 0.0f, wheelEnd, rotateBarOutline);
}

}
