#include "BrushTool.hpp"
#include <Helpers/ConvertVec.hpp>
#include "../DrawComponents/DrawBrushStroke.hpp"
#include "../GUIStuff/GUIManager.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "DrawingProgramToolBase.hpp"
#include <chrono>

#define VEL_SMOOTH_MIN 0.6
#define VEL_SMOOTH_MAX 1.0
#define MINIMUM_DISTANCE_TO_NEXT_POINT 0.002f

BrushTool::BrushTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType BrushTool::get_type() {
    return DrawingProgramToolType::BRUSH;
}

void BrushTool::switch_tool(DrawingProgramToolType newTool) {
    commit_stroke();
}

bool BrushTool::extensive_point_checking(const Vector2f& newPoint) {
    auto& points = controls.intermediateItem->d->points;
    if(points.size() >= 1 && (newPoint - points[points.size() - 1].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    if(points.size() >= 2 && (newPoint - points[points.size() - 2].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    if(points.size() >= 3 && (newPoint - points[points.size() - 3].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    return true;
}

bool BrushTool::extensive_point_checking_back(const Vector2f& newPoint) {
    auto& points = controls.intermediateItem->d->points;
    if(points.size() >= 2 && (newPoint - points[points.size() - 2].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    if(points.size() >= 3 && (newPoint - points[points.size() - 3].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    if(points.size() >= 4 && (newPoint - points[points.size() - 4].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    return true;
}

void BrushTool::tool_update() {
    if(drawP.controls.cursorHoveringOverCanvas)
        drawP.world.main.input.hideCursor = true;

    if(!controls.isDrawing)
        controls.penSmoothingData.clear();

    float averageScreenSize = (drawP.world.main.window.size.x() + drawP.world.main.window.size.y()) / 2.0f;
    float minVel = 0.2f * averageScreenSize;
    float maxVel = 3.5f * averageScreenSize;

    while(!controls.velocitySmoothingData.empty() && (std::chrono::steady_clock::now() - controls.velocitySmoothingData.front().t) > std::chrono::milliseconds(200))
        controls.velocitySmoothingData.pop_front();
    controls.velocitySmoothingData.emplace_back(
        drawP.world.main.input.mouse.move.norm() / drawP.world.main.deltaTime,
        std::chrono::steady_clock::now()
    );

    if(drawP.world.main.input.pen.isDown && drawP.controls.leftClickHeld && drawP.world.main.toolbar.tabletOptions.pressureAffectsBrushWidth) {
        while(!controls.penSmoothingData.empty() && (std::chrono::steady_clock::now() - controls.penSmoothingData.front().t) > std::chrono::milliseconds(static_cast<int>(drawP.world.main.toolbar.tabletOptions.smoothingSamplingTime * 1000.0f)))
            controls.penSmoothingData.pop_front();
        controls.penSmoothingData.emplace_back(
            drawP.world.main.input.pen.pressure,
            std::chrono::steady_clock::now()
        );
        float averagePressure = 0.0f;
        for(auto& [width, t] : controls.penSmoothingData)
            averagePressure += width;
        averagePressure /= controls.penSmoothingData.size();
        controls.penWidth = averagePressure;
    }
    else if(drawP.world.main.toolbar.velocityAffectsBrushWidth) {
        float averageVel = 0.0f;
        for(auto& [width, t] : controls.velocitySmoothingData)
            averageVel += width;
        averageVel /= controls.velocitySmoothingData.size();
        controls.penWidth = std::clamp<float>((averageVel - minVel) / (maxVel - minVel), VEL_SMOOTH_MIN, VEL_SMOOTH_MAX);
    }
    else
        controls.penWidth = 1.0f;

    bool isPenDown = false;
    if(drawP.world.main.input.pen.isDown && drawP.controls.leftClickHeld)
        isPenDown = controls.penWidth != 0.0;
    else if(drawP.controls.leftClickHeld)
        isPenDown = true;

    if(isPenDown) {
        if(!controls.isDrawing) {
            controls.isDrawing = true;
            controls.intermediateItem = std::make_shared<DrawBrushStroke>();
            DrawBrushStrokePoint p;
            p.pos = drawP.world.main.input.mouse.pos;
            p.width = drawP.controls.relativeWidth * controls.penWidth;
            controls.prevPointUnaltered = p.pos;
            controls.intermediateItem->d->points.emplace_back(p);
            controls.intermediateItem->d->color = drawP.controls.foregroundColor;
            controls.intermediateItem->d->hasRoundCaps = controls.hasRoundCaps;
            controls.intermediateItem->coords = drawP.world.drawData.cam.c;
            controls.intermediateItem->lastUpdateTime = std::chrono::steady_clock::now();
            uint64_t placement = drawP.components.client_list().size();
            auto objAdd = drawP.components.client_insert(placement, controls.intermediateItem);
            controls.intermediateItem->commit_update(drawP);
            controls.intermediateItem->client_send_place(drawP);
            // NOTE: The data isnt finallized at this point, so a BVH isn't generated for an object that you undo while youre drawing (this isnt just for brush strokes)
            // The fix for now is to generate a BVH for an object when you redo it
            drawP.add_undo_place_component(objAdd);
            controls.addedTemporaryPoint = false;
        }
        else {
            controls.intermediateItem->lastUpdateTime = std::chrono::steady_clock::now();

            DrawBrushStrokePoint p;
            p.pos = controls.intermediateItem->coords.get_mouse_pos(drawP.world);
            p.width = drawP.controls.relativeWidth * controls.penWidth;

            if(!controls.addedTemporaryPoint) {
                if(extensive_point_checking(p.pos)) {
                    controls.intermediateItem->d->points.emplace_back(p);
                    controls.addedTemporaryPoint = true;
                }
                else
                    controls.intermediateItem->d->points.back().width = std::max(controls.intermediateItem->d->points.back().width, p.width);
            }

            if(controls.addedTemporaryPoint) {
                const DrawBrushStrokePoint& prevP = controls.intermediateItem->d->points[controls.intermediateItem->d->points.size() - 2];
                float distToPrev = (p.pos - prevP.pos).norm();
                if(extensive_point_checking_back(p.pos)) {
                    controls.intermediateItem->d->points.back().pos = p.pos;
                    controls.intermediateItem->d->points.back().width = std::max(controls.intermediateItem->d->points.back().width, p.width);
                    controls.intermediateItem->d->points[controls.intermediateItem->d->points.size() - 2].width = std::max(controls.intermediateItem->d->points[controls.intermediateItem->d->points.size() - 2].width, p.width);
                }
                if((!controls.drawingMinimumRelativeToSize && distToPrev >= 10.0) || (controls.drawingMinimumRelativeToSize && distToPrev >= drawP.controls.relativeWidth * DRAW_MINIMUM_LIMIT)) {
                    controls.intermediateItem->d->points.back() = p;
                    controls.addedTemporaryPoint = false;
                    controls.intermediateItem->client_send_update(drawP, false);

                    if(controls.midwayInterpolation) {
                        if(controls.intermediateItem->d->points.size() != 2) // Don't interpolate the first point
                            controls.intermediateItem->d->points[controls.intermediateItem->d->points.size() - 2].pos = (controls.prevPointUnaltered + p.pos) * 0.5;
                        controls.prevPointUnaltered = p.pos;
                    }
                }
                controls.intermediateItem->commit_update(drawP);
            }
        }
    }
    else if(controls.isDrawing) {
        controls.isDrawing = false;
        commit_stroke();
    }
}

void BrushTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("brush tool");
    t.gui.text_label_centered("Brush");
    t.gui.checkbox_field("hasroundcaps", "Round Caps", &controls.hasRoundCaps);
    t.gui.slider_scalar_field("relwidth", "Size", &drawP.controls.relativeWidth, 3.0f, 40.0f);
    t.gui.pop_id();
}

bool BrushTool::prevent_undo_or_redo() {
    return controls.intermediateItem != nullptr;
}

void BrushTool::commit_stroke() {
    if(controls.intermediateItem && controls.intermediateItem->collabListInfo.lock()) {
        if(!controls.intermediateItem->d->points.empty()) {
            controls.intermediateItem->client_send_update(drawP, true);
            controls.intermediateItem->commit_update(drawP);
        }
    }
    controls.intermediateItem = nullptr; 
}

void BrushTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(!drawData.main->toolbar.io->hoverObstructed) {
        SkPaint linePaint;
        linePaint.setColor4f(drawP.world.canvasTheme.toolFrontColor);
        linePaint.setStroke(true);
        linePaint.setStrokeWidth(0.0f);
        float width;
        if(controls.isDrawing)
            width = drawP.controls.relativeWidth * controls.penWidth * 0.5f;
        else
            width = drawP.controls.relativeWidth * 0.5f;
        Vector2f pos = drawData.main->input.mouse.pos;
        canvas->drawCircle(pos.x(), pos.y(), width, linePaint);
    }
}
