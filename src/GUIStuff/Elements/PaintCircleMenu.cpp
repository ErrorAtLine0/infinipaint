#include "PaintCircleMenu.hpp"
#include "Helpers/ConvertVec.hpp"
#include <include/core/SkPathBuilder.h>
#include "../GUIManager.hpp"

namespace GUIStuff {

constexpr float ROTATE_START = 125.0f;
constexpr float PALETTE_START = 100.0f;
constexpr float CIRCLE_END = 150.0f;
constexpr double ROTATE_BAR_SNAP_DISTANCE = 0.2;
constexpr double ROTATE_BAR_SNAP_DISTRIBUTION = std::numbers::pi * 0.25;

PaintCircleMenu::PaintCircleMenu(GUIManager& gui):
    Element(gui) {}

void PaintCircleMenu::layout(const Data& data, const std::function<void()>& onChange) {
    d = data;
    this->onChange = onChange;

    CLAY_AUTO_ID({
        .layout = { 
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
        },
        .custom = { .customData = this }
    }) {
    }
}

bool PaintCircleMenu::collides_with_point(const Vector2f& p) const {
    return boundingBox.has_value() && SCollision::collide(SCollision::Circle<float>(boundingBox.value().center(), CIRCLE_END), p);
}

bool PaintCircleMenu::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) {
    isHovering = mouseHovering;
    isHeld = isHovering && button.button == InputManager::MouseButton::LEFT && button.down;
    update_paint_circle_menu_mouse(button.pos, button.button == InputManager::MouseButton::LEFT && button.down);
    return mouseHovering && collides_with_point(button.pos);
}

bool PaintCircleMenu::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) {
    update_paint_circle_menu_mouse(motion.pos, false);
    return mouseHovering && collides_with_point(motion.pos);
}

void PaintCircleMenu::update_paint_circle_menu_mouse(const Vector2f& p, bool leftClicked) {
    if(boundingBox.has_value()) {
        Vector2f vecFromCenter = (p - boundingBox.value().center()).normalized();
        float distFromCenter = vec_distance(p, boundingBox.value().center());
        isRotateBarHovered = isHovering && distFromCenter > ROTATE_START && distFromCenter < CIRCLE_END;
        isRotateBarHeld = isRotateBarHovered && isHeld;
        isColorBarHovered = isHovering && distFromCenter > PALETTE_START && distFromCenter < ROTATE_START;

        if(isColorBarHovered) {
            double selectionAngle = std::atan2(-vecFromCenter.y(), -vecFromCenter.x()) + std::numbers::pi;
            colorSelectionIndex = (selectionAngle / (std::numbers::pi * 2.0)) * d.palette.size();
            colorSelectionIndex = std::clamp<unsigned>(colorSelectionIndex, 0, d.palette.size() - 1);
        }

        if(isRotateBarHeld) {
            gui.set_post_callback_func([&, vecFromCenter] {
                auto& rotationAngle = *d.rotationAngle;
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
                    d.onRotate();
                }
            });
        }
        else if(isColorBarHovered && leftClicked) {
            gui.set_post_callback_func([&] {
                Vector4f& selectedColor = *d.selectedColor;
                selectedColor.x() = d.palette[colorSelectionIndex].x();
                selectedColor.y() = d.palette[colorSelectionIndex].y();
                selectedColor.z() = d.palette[colorSelectionIndex].z();
                d.onPaletteClick();
            });
        }
    }
}

void PaintCircleMenu::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) {
    auto& bb = boundingBox.value();

    canvas->save();
    canvas->translate(bb.center().x(), bb.center().y());

    draw_rotate_bar(canvas, io, skiaAA);
    draw_palette_bar(canvas, io, skiaAA);
    SkPaint p;
    p.setAntiAlias(skiaAA);
    p.setColor4f(color_mul_alpha(io.theme->backColor1, 0.5f));
    canvas->drawCircle(0.0f, 0.0f, PALETTE_START, p);

    canvas->restore();
}

void PaintCircleMenu::draw_rotate_bar(SkCanvas* canvas, UpdateInputData& io, bool skiaAA) {
    double rotationAngle = *d.rotationAngle;
    float rotateBarMiddleRadius = (CIRCLE_END + ROTATE_START) / 2.0f;
    SkPaint rotateBarFill(io.theme->backColor2);
    rotateBarFill.setAntiAlias(skiaAA);
    rotateBarFill.setStroke(true);
    rotateBarFill.setStrokeWidth(CIRCLE_END - ROTATE_START);
    canvas->drawCircle(0.0f, 0.0f, rotateBarMiddleRadius, rotateBarFill);

    if(isRotateBarHovered) {
        for(double snapPos = 0.0; snapPos < std::numbers::pi * 2.0; snapPos += ROTATE_BAR_SNAP_DISTRIBUTION) {
            Vector2f lineDir{std::cos(snapPos), -std::sin(snapPos)};
            SkPaint p(snapPos == 0.0 ? io.theme->fillColor1 : io.theme->frontColor2);
            p.setAntiAlias(skiaAA);
            p.setStrokeWidth(2.0f);
            canvas->drawLine(lineDir.x() * ROTATE_START, lineDir.y() * ROTATE_START, lineDir.x() * CIRCLE_END, lineDir.y() * CIRCLE_END, p);
        }
    }

    SkPaint rotateBarHolderFill;
    rotateBarHolderFill.setAntiAlias(skiaAA);
    if(isRotateBarHeld)
        rotateBarHolderFill.setColor4f(io.theme->fillColor1);
    else if(isRotateBarHovered)
        rotateBarHolderFill.setColor4f(io.theme->fillColor2);
    else
        rotateBarHolderFill.setColor4f(io.theme->frontColor1);
    float rotateBarHolderRadius = (CIRCLE_END - ROTATE_START) / 2.0f;
    Vector2f rotateBarHolderPos{std::cos(rotationAngle) * rotateBarMiddleRadius, -std::sin(rotationAngle) * rotateBarMiddleRadius};
    canvas->drawCircle(rotateBarHolderPos.x(), rotateBarHolderPos.y(), rotateBarHolderRadius, rotateBarHolderFill);
    SkPaint rotateBarHolderOutline(io.theme->frontColor1);
    rotateBarHolderOutline.setAntiAlias(skiaAA);
    rotateBarHolderOutline.setStroke(true);
    rotateBarHolderOutline.setStrokeWidth(2.0f);
    canvas->drawCircle(rotateBarHolderPos.x(), rotateBarHolderPos.y(), rotateBarHolderRadius, rotateBarHolderOutline);

    SkPaint rotateBarOutline(io.theme->frontColor2);
    rotateBarOutline.setAntiAlias(skiaAA);
    rotateBarOutline.setStroke(true);
    rotateBarOutline.setStrokeWidth(2.0f);
    canvas->drawCircle(0.0f, 0.0f, ROTATE_START, rotateBarOutline);
    canvas->drawCircle(0.0f, 0.0f, CIRCLE_END, rotateBarOutline);
}

void PaintCircleMenu::draw_palette_bar(SkCanvas* canvas, UpdateInputData& io, bool skiaAA) {
    if(d.palette.empty() || !d.selectedColor)
        return;

    float middleRadius = (PALETTE_START + ROTATE_START) / 2.0f;

    double colorDistribution = std::numbers::pi * 2.0 / d.palette.size();
    SkRect colorOval = SkRect::MakeLTRB(-middleRadius, -middleRadius, middleRadius, middleRadius);
    for(unsigned i = 0; i < d.palette.size(); i++) {
        double angleStart = i * colorDistribution;
        double angleEnd = (i + 1) * colorDistribution;

        SkPaint colorP(SkColor4f{d.palette[i].x(), d.palette[i].y(), d.palette[i].z(), 1.0f});
        colorP.setAntiAlias(skiaAA);
        colorP.setStroke(true);
        colorP.setStrokeWidth(ROTATE_START - PALETTE_START);
        canvas->drawArc(colorOval, angleStart * 180.0 / std::numbers::pi, (angleEnd - angleStart) * 180.0 / std::numbers::pi, false, colorP);

        Vector2f lineDir1{std::cos(angleStart), std::sin(angleStart)};
        SkPaint lineP(io.theme->frontColor2);
        lineP.setAntiAlias(skiaAA);
        lineP.setStrokeWidth(2.0f);
        lineP.setStroke(true);
        canvas->drawLine(lineDir1.x() * PALETTE_START, lineDir1.y() * PALETTE_START, lineDir1.x() * ROTATE_START, lineDir1.y() * ROTATE_START, lineP);
    }

    SkPaint barOutline(io.theme->frontColor2);
    barOutline.setAntiAlias(skiaAA);
    barOutline.setStroke(true);
    barOutline.setStrokeWidth(2.0f);
    canvas->drawCircle(0.0f, 0.0f, PALETTE_START, barOutline);
    canvas->drawCircle(0.0f, 0.0f, ROTATE_START, barOutline);

    SkRect startOval = SkRect::MakeLTRB(-PALETTE_START, -PALETTE_START, PALETTE_START, PALETTE_START);
    SkRect endOval = SkRect::MakeLTRB(-ROTATE_START, -ROTATE_START, ROTATE_START, ROTATE_START);
    if(isColorBarHovered) {
        SkPathBuilder selectionPathBuilder;
        double angleStart = colorSelectionIndex * colorDistribution;
        double angleEnd = (colorSelectionIndex + 1) * colorDistribution;
        Vector2f lineDir1{std::cos(angleStart), std::sin(angleStart)};
        Vector2f lineDir2{std::cos(angleEnd), std::sin(angleEnd)};
        SkPaint lineP(io.theme->fillColor1);
        lineP.setAntiAlias(skiaAA);
        lineP.setStrokeWidth(5.0f);
        lineP.setStroke(true);
        lineP.setStrokeJoin(SkPaint::kRound_Join);
        selectionPathBuilder.moveTo(lineDir1.x() * PALETTE_START, lineDir1.y() * PALETTE_START);
        selectionPathBuilder.lineTo(lineDir1.x() * ROTATE_START, lineDir1.y() * ROTATE_START);
        selectionPathBuilder.arcTo(endOval, angleStart * 180.0 / std::numbers::pi, (angleEnd - angleStart) * 180.0 / std::numbers::pi, false);
        selectionPathBuilder.lineTo(lineDir2.x() * PALETTE_START, lineDir2.y() * PALETTE_START);
        selectionPathBuilder.arcTo(startOval, angleEnd * 180.0 / std::numbers::pi, (angleStart - angleEnd) * 180.0 / std::numbers::pi, false);
        selectionPathBuilder.close();
        canvas->drawPath(selectionPathBuilder.detach(), lineP);
    }
    for(unsigned i = 0; i < d.palette.size(); i++) {
        if(d.palette[i].x() == d.selectedColor->x() && d.palette[i].y() == d.selectedColor->y() && d.palette[i].z() == d.selectedColor->z()) {
            SkPathBuilder selectionPathBuilder;
            double angleStart = i * colorDistribution;
            double angleEnd = (i + 1) * colorDistribution;
            Vector2f lineDir1{std::cos(angleStart), std::sin(angleStart)};
            Vector2f lineDir2{std::cos(angleEnd), std::sin(angleEnd)};
            SkPaint lineP(io.theme->fillColor1);
            lineP.setAntiAlias(skiaAA);
            lineP.setStrokeWidth(5.0f);
            lineP.setStroke(true);
            lineP.setStrokeJoin(SkPaint::kRound_Join);
            selectionPathBuilder.moveTo(lineDir1.x() * PALETTE_START, lineDir1.y() * PALETTE_START);
            selectionPathBuilder.lineTo(lineDir1.x() * ROTATE_START, lineDir1.y() * ROTATE_START);
            selectionPathBuilder.arcTo(endOval, angleStart * 180.0 / std::numbers::pi, (angleEnd - angleStart) * 180.0 / std::numbers::pi, false);
            selectionPathBuilder.lineTo(lineDir2.x() * PALETTE_START, lineDir2.y() * PALETTE_START);
            selectionPathBuilder.arcTo(startOval, angleEnd * 180.0 / std::numbers::pi, (angleStart - angleEnd) * 180.0 / std::numbers::pi, false);
            selectionPathBuilder.close();
            canvas->drawPath(selectionPathBuilder.detach(), lineP);
            break;
        }
    }
}

}
