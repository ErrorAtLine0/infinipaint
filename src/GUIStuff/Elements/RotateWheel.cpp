#include "RotateWheel.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff {

constexpr float WHEEL_WIDTH = 13.0f;

constexpr double ROTATE_BAR_SNAP_DISTANCE = 0.3;
constexpr double ROTATE_BAR_SNAP_DISTRIBUTION = std::numbers::pi * 0.25;

void RotateWheel::update(UpdateInputData& io, double* newRotationAnglePtr, const std::function<void()>& elemUpdate) {
    rotateAngle = 0.0f;
    if(!newRotationAnglePtr)
        return;

    CLAY({.layout = { 
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
        },
        .custom = { .customData = this }
    }) {
        selection.update(Clay_Hovered(), io.mouse.leftClick, io.mouse.leftHeld);
        Vector2f vecFromCenter = (io.mouse.pos - bb.center()).normalized();
        float distFromCenter = vec_distance(io.mouse.pos, bb.center());
        wheelHovered = selection.hovered && distFromCenter > wheel_start() && distFromCenter < wheel_end();
        if(selection.held) {
            double& newRotationAngle = *newRotationAnglePtr;
            newRotationAngle = std::atan2(vecFromCenter.y(), -vecFromCenter.x()) + std::numbers::pi;
            if(wheelHovered) { // If we're hovering over the rotation bar, we should try snapping to specific angles
                for(double snapPos = 0.0; snapPos < std::numbers::pi * 2.0 + 0.001; snapPos += ROTATE_BAR_SNAP_DISTRIBUTION) {
                    if(std::fabs(newRotationAngle - snapPos) < ROTATE_BAR_SNAP_DISTANCE) {
                        newRotationAngle = snapPos;
                        if(newRotationAngle >= std::numbers::pi * 2.0 - 0.001) // Set 2pi back to 0
                            newRotationAngle = 0;
                        break;
                    }
                }
            }
        }
        if(elemUpdate)
            elemUpdate();
    }

    rotateAngle = *newRotationAnglePtr;
}

float RotateWheel::wheel_start() {
    return wheel_end() - WHEEL_WIDTH;
}

float RotateWheel::wheel_end() {
    return bb.width() * 0.5f;
}

void RotateWheel::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
    bb = get_bb(command);
    canvas->save();
    canvas->translate(bb.center().x(), bb.center().y());

    draw_rotate_wheel(canvas, io);

    canvas->restore();
}

void RotateWheel::draw_rotate_wheel(SkCanvas* canvas, UpdateInputData& io) {
    float wheelStart = wheel_start();
    float wheelEnd = wheel_end();

    float rotateBarMiddleRadius = (wheelEnd + wheelStart) / 2.0f;
    SkPaint rotateBarFill(io.theme->backColor2);
    rotateBarFill.setStroke(true);
    rotateBarFill.setStrokeWidth(wheelEnd - wheelStart);
    canvas->drawCircle(0.0f, 0.0f, rotateBarMiddleRadius, rotateBarFill);

    if(wheelHovered) {
        for(double snapPos = 0.0; snapPos < std::numbers::pi * 2.0; snapPos += ROTATE_BAR_SNAP_DISTRIBUTION) {
            Vector2f lineDir{std::cos(snapPos), -std::sin(snapPos)};
            SkPaint p(snapPos == 0.0 ? io.theme->fillColor1 : io.theme->frontColor2);
            p.setStrokeWidth(1.0f);
            canvas->drawLine(lineDir.x() * wheelStart, lineDir.y() * wheelStart, lineDir.x() * wheelEnd, lineDir.y() * wheelEnd, p);
        }
    }

    SkPaint rotateBarHolderFill;
    if(selection.held)
        rotateBarHolderFill.setColor4f(io.theme->fillColor1);
    else if(selection.hovered)
        rotateBarHolderFill.setColor4f(io.theme->fillColor2);
    else
        rotateBarHolderFill.setColor4f(io.theme->frontColor2);
    float rotateBarHolderRadius = (wheelEnd - wheelStart) / 2.0f;
    Vector2f rotateBarHolderPos{std::cos(rotateAngle) * rotateBarMiddleRadius, -std::sin(rotateAngle) * rotateBarMiddleRadius};
    canvas->drawCircle(rotateBarHolderPos.x(), rotateBarHolderPos.y(), rotateBarHolderRadius, rotateBarHolderFill);
    SkPaint rotateBarHolderOutline(io.theme->frontColor1);
    rotateBarHolderOutline.setStroke(true);
    rotateBarHolderOutline.setStrokeWidth(2.0f);
    canvas->drawCircle(rotateBarHolderPos.x(), rotateBarHolderPos.y(), rotateBarHolderRadius, rotateBarHolderOutline);

    SkPaint rotateBarOutline(io.theme->frontColor2);
    rotateBarOutline.setStroke(true);
    rotateBarOutline.setStrokeWidth(1.0f);
    canvas->drawCircle(0.0f, 0.0f, wheelStart, rotateBarOutline);
    canvas->drawCircle(0.0f, 0.0f, wheelEnd, rotateBarOutline);
}

}
