#include "PaintCircleMenu.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff {

constexpr float ROTATE_START = 125.0f;
constexpr float PALETTE_START = 100.0f;
constexpr float CIRCLE_END = 150.0f;
constexpr double ROTATE_BAR_SNAP_DISTANCE = 0.2;
constexpr double ROTATE_BAR_SNAP_DISTRIBUTION = std::numbers::pi * 0.25;

void PaintCircleMenu::update(UpdateInputData& io, const Data& data, const std::function<void()>& elemUpdate) {
    CLAY({.layout = { 
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
        },
        .custom = { .customData = this }
    }) {
        bool isHoveredNoObstruction = Clay_Hovered() && !io.hoverObstructed;
        Vector2f vecFromCenter = (io.mouse.pos - bb.center()).normalized();
        float distFromCenter = vec_distance(io.mouse.pos, bb.center());
        bool rotateHovered = false;
        if(data.newRotationAngle) {
            rotateBarSelect.update(isHoveredNoObstruction && distFromCenter > ROTATE_START && distFromCenter < CIRCLE_END, io.mouse.leftClick, io.mouse.leftHeld);
            if(rotateBarSelect.held) {
                double& newRotationAngle = *data.newRotationAngle;
                newRotationAngle = std::atan2(vecFromCenter.y(), -vecFromCenter.x()) + std::numbers::pi;
                if(rotateBarSelect.hovered) { // If we're hovering over the rotation bar, we should try snapping to specific angles
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
            else
                *data.newRotationAngle = data.currentRotationAngle;
            rotateHovered = rotateBarSelect.hovered;
        }
        bool colorHovered = false;
        if(!data.palette.empty() && data.selectedColor) {
            colorBarSelect.update(isHoveredNoObstruction && distFromCenter > PALETTE_START && distFromCenter < ROTATE_START, io.mouse.leftClick, io.mouse.leftHeld);
            if(colorBarSelect.hovered) {
                Vector4f& selectedColor = *data.selectedColor;
                double selectionAngle = std::atan2(-vecFromCenter.y(), -vecFromCenter.x()) + std::numbers::pi;
                colorSelectionIndex = (selectionAngle / (std::numbers::pi * 2.0)) * data.palette.size();
                colorSelectionIndex = std::clamp<unsigned>(colorSelectionIndex, 0, data.palette.size() - 1);
                if(colorBarSelect.clicked) {
                    selectedColor.x() = data.palette[colorSelectionIndex].x();
                    selectedColor.y() = data.palette[colorSelectionIndex].y();
                    selectedColor.z() = data.palette[colorSelectionIndex].z();
                }
            }
            colorHovered = colorBarSelect.hovered;
        }
        if(rotateHovered || colorHovered)
            io.hoverObstructed = true;
        if(elemUpdate)
            elemUpdate();
    }

    d = data;
}

void PaintCircleMenu::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
    bb = get_bb(command);
    canvas->save();
    canvas->translate(bb.center().x(), bb.center().y());

    draw_rotate_bar(canvas, io);
    draw_palette_bar(canvas, io);

    canvas->restore();
}

void PaintCircleMenu::draw_rotate_bar(SkCanvas* canvas, UpdateInputData& io) {
    if(!d.newRotationAngle)
        return;

    double rotationAngle = d.currentRotationAngle;
    float rotateBarMiddleRadius = (CIRCLE_END + ROTATE_START) / 2.0f;
    SkPaint rotateBarFill(io.theme->backColor2);
    rotateBarFill.setStroke(true);
    rotateBarFill.setStrokeWidth(CIRCLE_END - ROTATE_START);
    canvas->drawCircle(0.0f, 0.0f, rotateBarMiddleRadius, rotateBarFill);

    if(rotateBarSelect.hovered) {
        for(double snapPos = 0.0; snapPos < std::numbers::pi * 2.0; snapPos += ROTATE_BAR_SNAP_DISTRIBUTION) {
            Vector2f lineDir{std::cos(snapPos), -std::sin(snapPos)};
            SkPaint p(snapPos == 0.0 ? io.theme->fillColor1 : io.theme->frontColor2);
            p.setStrokeWidth(2.0f);
            canvas->drawLine(lineDir.x() * ROTATE_START, lineDir.y() * ROTATE_START, lineDir.x() * CIRCLE_END, lineDir.y() * CIRCLE_END, p);
        }
    }

    SkPaint rotateBarHolderFill;
    if(rotateBarSelect.held)
        rotateBarHolderFill.setColor4f(io.theme->fillColor1);
    else if(rotateBarSelect.hovered)
        rotateBarHolderFill.setColor4f(io.theme->fillColor2);
    else
        rotateBarHolderFill.setColor4f(io.theme->frontColor1);
    float rotateBarHolderRadius = (CIRCLE_END - ROTATE_START) / 2.0f;
    Vector2f rotateBarHolderPos{std::cos(rotationAngle) * rotateBarMiddleRadius, -std::sin(rotationAngle) * rotateBarMiddleRadius};
    canvas->drawCircle(rotateBarHolderPos.x(), rotateBarHolderPos.y(), rotateBarHolderRadius, rotateBarHolderFill);
    SkPaint rotateBarHolderOutline(io.theme->frontColor1);
    rotateBarHolderOutline.setStroke(true);
    rotateBarHolderOutline.setStrokeWidth(2.0f);
    canvas->drawCircle(rotateBarHolderPos.x(), rotateBarHolderPos.y(), rotateBarHolderRadius, rotateBarHolderOutline);

    SkPaint rotateBarOutline(io.theme->frontColor2);
    rotateBarOutline.setStroke(true);
    rotateBarOutline.setStrokeWidth(2.0f);
    canvas->drawCircle(0.0f, 0.0f, ROTATE_START, rotateBarOutline);
    canvas->drawCircle(0.0f, 0.0f, CIRCLE_END, rotateBarOutline);
}

void PaintCircleMenu::draw_palette_bar(SkCanvas* canvas, UpdateInputData& io) {
    if(d.palette.empty() || !d.selectedColor)
        return;

    float middleRadius = (PALETTE_START + ROTATE_START) / 2.0f;

    double colorDistribution = std::numbers::pi * 2.0 / d.palette.size();
    SkRect colorOval = SkRect::MakeLTRB(-middleRadius, -middleRadius, middleRadius, middleRadius);
    for(unsigned i = 0; i < d.palette.size(); i++) {
        double angleStart = i * colorDistribution;
        double angleEnd = (i + 1) * colorDistribution;

        SkPaint colorP(SkColor4f{d.palette[i].x(), d.palette[i].y(), d.palette[i].z(), 1.0f});
        colorP.setStroke(true);
        colorP.setStrokeWidth(ROTATE_START - PALETTE_START);
        canvas->drawArc(colorOval, angleStart * 180.0 / std::numbers::pi, (angleEnd - angleStart) * 180.0 / std::numbers::pi, false, colorP);

        Vector2f lineDir1{std::cos(angleStart), std::sin(angleStart)};
        SkPaint lineP(io.theme->frontColor2);
        lineP.setStrokeWidth(2.0f);
        lineP.setStroke(true);
        canvas->drawLine(lineDir1.x() * PALETTE_START, lineDir1.y() * PALETTE_START, lineDir1.x() * ROTATE_START, lineDir1.y() * ROTATE_START, lineP);
    }

    SkPaint barOutline(io.theme->frontColor2);
    barOutline.setStroke(true);
    barOutline.setStrokeWidth(2.0f);
    canvas->drawCircle(0.0f, 0.0f, PALETTE_START, barOutline);
    canvas->drawCircle(0.0f, 0.0f, ROTATE_START, barOutline);

    SkRect startOval = SkRect::MakeLTRB(-PALETTE_START, -PALETTE_START, PALETTE_START, PALETTE_START);
    SkRect endOval = SkRect::MakeLTRB(-ROTATE_START, -ROTATE_START, ROTATE_START, ROTATE_START);
    if(colorBarSelect.hovered) {
        SkPath selectionPath;
        double angleStart = colorSelectionIndex * colorDistribution;
        double angleEnd = (colorSelectionIndex + 1) * colorDistribution;
        Vector2f lineDir1{std::cos(angleStart), std::sin(angleStart)};
        Vector2f lineDir2{std::cos(angleEnd), std::sin(angleEnd)};
        SkPaint lineP(io.theme->fillColor1);
        lineP.setStrokeWidth(5.0f);
        lineP.setStroke(true);
        lineP.setStrokeJoin(SkPaint::kRound_Join);
        selectionPath.moveTo(lineDir1.x() * PALETTE_START, lineDir1.y() * PALETTE_START);
        selectionPath.lineTo(lineDir1.x() * ROTATE_START, lineDir1.y() * ROTATE_START);
        selectionPath.arcTo(endOval, angleStart * 180.0 / std::numbers::pi, (angleEnd - angleStart) * 180.0 / std::numbers::pi, false);
        selectionPath.lineTo(lineDir2.x() * PALETTE_START, lineDir2.y() * PALETTE_START);
        selectionPath.arcTo(startOval, angleEnd * 180.0 / std::numbers::pi, (angleStart - angleEnd) * 180.0 / std::numbers::pi, false);
        selectionPath.close();
        canvas->drawPath(selectionPath, lineP);
    }
    for(unsigned i = 0; i < d.palette.size(); i++) {
        if(d.palette[i].x() == d.selectedColor->x() && d.palette[i].y() == d.selectedColor->y() && d.palette[i].z() == d.selectedColor->z()) {
            SkPath selectionPath;
            double angleStart = i * colorDistribution;
            double angleEnd = (i + 1) * colorDistribution;
            Vector2f lineDir1{std::cos(angleStart), std::sin(angleStart)};
            Vector2f lineDir2{std::cos(angleEnd), std::sin(angleEnd)};
            SkPaint lineP(io.theme->fillColor1);
            lineP.setStrokeWidth(5.0f);
            lineP.setStroke(true);
            lineP.setStrokeJoin(SkPaint::kRound_Join);
            selectionPath.moveTo(lineDir1.x() * PALETTE_START, lineDir1.y() * PALETTE_START);
            selectionPath.lineTo(lineDir1.x() * ROTATE_START, lineDir1.y() * ROTATE_START);
            selectionPath.arcTo(endOval, angleStart * 180.0 / std::numbers::pi, (angleEnd - angleStart) * 180.0 / std::numbers::pi, false);
            selectionPath.lineTo(lineDir2.x() * PALETTE_START, lineDir2.y() * PALETTE_START);
            selectionPath.arcTo(startOval, angleEnd * 180.0 / std::numbers::pi, (angleStart - angleEnd) * 180.0 / std::numbers::pi, false);
            selectionPath.close();
            canvas->drawPath(selectionPath, lineP);
            break;
        }
    }
}

}
