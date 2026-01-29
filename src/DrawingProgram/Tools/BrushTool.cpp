#include "BrushTool.hpp"
#include <Helpers/ConvertVec.hpp>
#include "../../GUIStuff/GUIManager.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "DrawingProgramToolBase.hpp"
#include "../../CanvasComponents/BrushStrokeCanvasComponent.hpp"
#include "Helpers/NetworkingObjects/NetObjTemporaryPtr.decl.hpp"
#include "Helpers/NetworkingObjects/NetObjWeakPtr.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"
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

void BrushTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    if(objInfoBeingEdited == erasedComp)
        objInfoBeingEdited = nullptr;
}

bool BrushTool::extensive_point_checking(const BrushStrokeCanvasComponent& brushStroke, const Vector2f& newPoint) {
    auto& points = *brushStroke.d.points;
    if(points.size() >= 1 && (newPoint - points[points.size() - 1].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    if(points.size() >= 2 && (newPoint - points[points.size() - 2].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    if(points.size() >= 3 && (newPoint - points[points.size() - 3].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    return true;
}

bool BrushTool::extensive_point_checking_back(const BrushStrokeCanvasComponent& brushStroke, const Vector2f& newPoint) {
    auto& points = *brushStroke.d.points;
    if(points.size() >= 2 && (newPoint - points[points.size() - 2].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    if(points.size() >= 3 && (newPoint - points[points.size() - 3].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    if(points.size() >= 4 && (newPoint - points[points.size() - 4].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    return true;
}

void BrushTool::tool_update() {
    auto& toolConfig = drawP.world.main.toolConfig;

    if(drawP.controls.cursorHoveringOverCanvas)
        drawP.world.main.input.hideCursor = true;

    if(!objInfoBeingEdited)
        penSmoothingData.clear();

    if(drawP.world.main.input.pen.isDown && drawP.controls.leftClickHeld && drawP.world.main.toolbar.tabletOptions.pressureAffectsBrushWidth) {
        while(!penSmoothingData.empty() && (std::chrono::steady_clock::now() - penSmoothingData.front().t) > std::chrono::milliseconds(static_cast<int>(drawP.world.main.toolbar.tabletOptions.smoothingSamplingTime * 1000.0f)))
            penSmoothingData.pop_front();
        penSmoothingData.emplace_back(
            drawP.world.main.input.pen.pressure,
            std::chrono::steady_clock::now()
        );
        float averagePressure = 0.0f;
        for(auto& [width, t] : penSmoothingData)
            averagePressure += width;
        averagePressure /= penSmoothingData.size();
        penWidth = averagePressure;
        if(penWidth != 0.0f) {
            float brushMinSize = drawP.world.main.toolbar.tabletOptions.brushMinimumSize;
            penWidth = brushMinSize + penWidth * (1.0f - brushMinSize);
        }
    }
    else
        penWidth = 1.0f;

    bool isPenDown = false;
    if(drawP.world.main.input.pen.isDown && drawP.controls.leftClickHeld)
        isPenDown = penWidth != 0.0;
    else if(drawP.controls.leftClickHeld)
        isPenDown = true;

    if(isPenDown) {
        if(!objInfoBeingEdited) {
            if(drawP.layerMan.is_a_layer_being_edited()) {
                CanvasComponentContainer* newBrushStrokeContainer = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::BRUSHSTROKE);
                BrushStrokeCanvasComponent& newBrushStroke = static_cast<BrushStrokeCanvasComponent&>(newBrushStrokeContainer->get_comp());

                BrushStrokeCanvasComponentPoint p;
                p.pos = drawP.world.main.input.mouse.pos;
                p.width = toolConfig.get_relative_width(drawP, drawP.world.drawData.cam.c.inverseScale, toolConfig.brush.relativeWidth) * penWidth;
                prevPointUnaltered = p.pos;
                newBrushStroke.d.points->emplace_back(p);
                newBrushStroke.d.color = toolConfig.globalConf.foregroundColor;
                newBrushStroke.d.hasRoundCaps = toolConfig.brush.hasRoundCaps;
                newBrushStrokeContainer->coords = drawP.world.drawData.cam.c;
                objInfoBeingEdited = drawP.layerMan.add_component_to_layer_being_edited(newBrushStrokeContainer);
                addedTemporaryPoint = false;
            }
        }
        else {
            NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
            BrushStrokeCanvasComponent& brushStroke = static_cast<BrushStrokeCanvasComponent&>(containerPtr->get_comp());
            auto& brushPoints = *brushStroke.d.points;

            BrushStrokeCanvasComponentPoint p;
            p.pos = containerPtr->coords.get_mouse_pos(drawP.world);
            p.width = toolConfig.get_relative_width(drawP, containerPtr->coords.inverseScale, toolConfig.brush.relativeWidth) * penWidth;

            if(!addedTemporaryPoint) {
                if(extensive_point_checking(brushStroke, p.pos)) {
                    brushPoints.emplace_back(p);
                    addedTemporaryPoint = true;
                }
                else
                    brushPoints.back().width = std::max(brushPoints.back().width, p.width);
            }

            if(addedTemporaryPoint) {
                const BrushStrokeCanvasComponentPoint& prevP = brushPoints[brushPoints.size() - 2];
                float distToPrev = (p.pos - prevP.pos).norm();
                if(extensive_point_checking_back(brushStroke, p.pos)) {
                    brushPoints.back().pos = p.pos;
                    brushPoints.back().width = std::max(brushPoints.back().width, p.width);
                    brushPoints[brushPoints.size() - 2].width = std::max(brushPoints[brushPoints.size() - 2].width, p.width);
                }
                if((!drawingMinimumRelativeToSize && distToPrev >= 10.0) || (drawingMinimumRelativeToSize && distToPrev >= toolConfig.get_relative_width(drawP, containerPtr->coords.inverseScale, toolConfig.brush.relativeWidth) * BrushStrokeCanvasComponent::DRAW_MINIMUM_LIMIT)) {
                    brushPoints.back() = p;
                    addedTemporaryPoint = false;

                    if(midwayInterpolation) {
                        if(brushPoints.size() != 2) // Don't interpolate the first point
                            brushPoints[brushPoints.size() - 2].pos = (prevPointUnaltered + p.pos) * 0.5;
                        prevPointUnaltered = p.pos;
                    }
                }
            }

            containerPtr->send_comp_update(drawP, false);
            containerPtr->commit_update(drawP);
        }
    }
    else if(objInfoBeingEdited)
        commit_stroke();
}

void BrushTool::commit_stroke() {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        containerPtr->commit_update(drawP);
        containerPtr->send_comp_update(drawP, true);
        drawP.layerMan.add_undo_place_component(objInfoBeingEdited);
        objInfoBeingEdited = nullptr;
    }
}

void BrushTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("brush tool");
    t.gui.text_label_centered("Brush");
    t.gui.checkbox_field("hasroundcaps", "Round Caps", &drawP.world.main.toolConfig.brush.hasRoundCaps);
    drawP.world.main.toolConfig.relative_width_gui(drawP, "Size", &drawP.world.main.toolConfig.brush.relativeWidth);
    t.gui.pop_id();
}

bool BrushTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
    return true;
}

bool BrushTool::prevent_undo_or_redo() {
    return objInfoBeingEdited;
}

void BrushTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(!drawData.main->toolbar.io->hoverObstructed) {
        SkPaint linePaint;
        linePaint.setColor4f(drawP.world.canvasTheme.get_tool_front_color());
        linePaint.setStroke(true);
        linePaint.setStrokeWidth(0.0f);
        float width = drawP.world.main.toolConfig.get_relative_width(drawP, drawP.world.drawData.cam.c.inverseScale, drawP.world.main.toolConfig.brush.relativeWidth);
        if(objInfoBeingEdited)
            width *= penWidth * 0.5f;
        else
            width *= 0.5f;
        Vector2f pos = drawData.main->input.mouse.pos;
        canvas->drawCircle(pos.x(), pos.y(), width, linePaint);
    }
}
