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

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/CheckBoxHelpers.hpp"

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

void BrushTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        auto& toolConfig = drawP.world.main.toolConfig;
        if(button.down && drawP.layerMan.is_a_layer_being_edited() && !objInfoBeingEdited) {
            if(drawP.world.main.input.pen.isDown) {
                penWidth = drawP.world.main.input.pen.pressure;
                if(penWidth != 0.0f) {
                    float brushMinSize = drawP.world.main.toolbar.tabletOptions.brushMinimumSize;
                    penWidth = brushMinSize + penWidth * (1.0f - brushMinSize);
                }
            }
            else
                penWidth = 1.0f;

            auto relativeWidthResult = drawP.world.main.toolConfig.get_relative_width_stroke_size(drawP, drawP.world.drawData.cam.c.inverseScale);
            if(!relativeWidthResult.first.has_value()) {
                drawP.world.main.toolConfig.print_relative_width_fail_message(relativeWidthResult.second);
                return;
            }
            float width = relativeWidthResult.first.value() * penWidth;

            CanvasComponentContainer* newBrushStrokeContainer = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::BRUSHSTROKE);
            BrushStrokeCanvasComponent& newBrushStroke = static_cast<BrushStrokeCanvasComponent&>(newBrushStrokeContainer->get_comp());

            BrushStrokeCanvasComponentPoint p;
            p.pos = button.pos;
            p.width = width;
            prevPointUnaltered = p.pos;
            newBrushStroke.d.points->emplace_back(p);
            newBrushStroke.d.color = toolConfig.globalConf.foregroundColor;
            newBrushStroke.d.hasRoundCaps = toolConfig.brush.hasRoundCaps;
            newBrushStrokeContainer->coords = drawP.world.drawData.cam.c;
            objInfoBeingEdited = drawP.layerMan.add_component_to_layer_being_edited(newBrushStrokeContainer);
            addedTemporaryPoint = false;
        }
        else if(!button.down && objInfoBeingEdited)
            commit_stroke();
    }
}

void BrushTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(objInfoBeingEdited) {
        auto& toolConfig = drawP.world.main.toolConfig;
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        BrushStrokeCanvasComponent& brushStroke = static_cast<BrushStrokeCanvasComponent&>(containerPtr->get_comp());
        auto& brushPoints = *brushStroke.d.points;
        float width = toolConfig.get_relative_width_stroke_size(drawP, containerPtr->coords.inverseScale).first.value() * penWidth;

        BrushStrokeCanvasComponentPoint p;
        p.pos = containerPtr->coords.from_cam_space_to_this(drawP.world, motion.pos);
        p.width = width;

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
            if((!drawingMinimumRelativeToSize && distToPrev >= 10.0) || (drawingMinimumRelativeToSize && distToPrev >= width * BrushStrokeCanvasComponent::DRAW_MINIMUM_LIMIT)) {
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

void BrushTool::input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) {
    if(axis.axis == SDL_PEN_AXIS_PRESSURE) {
        penWidth = axis.value;
        if(penWidth != 0.0f) {
            float brushMinSize = drawP.world.main.toolbar.tabletOptions.brushMinimumSize;
            penWidth = brushMinSize + penWidth * (1.0f - brushMinSize);
        }
    }
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
    if(drawP.controls.cursorHoveringOverCanvas)
        drawP.world.main.input.hideCursor = true;
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
    using namespace GUIStuff;
    using namespace ElementHelpers;

    Toolbar& t = drawP.world.main.toolbar;
    t.gui.new_id("brush tool", [&] {
        text_label_centered(t.gui, "Brush");
        checkbox_boolean_field(t.gui, "hasroundcaps", "Round Caps", &drawP.world.main.toolConfig.brush.hasRoundCaps);
        drawP.world.main.toolConfig.relative_width_gui(drawP, "Size");
    });
}

void BrushTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
}

bool BrushTool::prevent_undo_or_redo() {
    return objInfoBeingEdited;
}

void BrushTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(!drawP.world.main.input.isTouchDevice && drawData.main->world->drawProg.controls.cursorHoveringOverCanvas) {
        auto relativeWidthResult = drawP.world.main.toolConfig.get_relative_width_stroke_size(drawP, drawP.world.drawData.cam.c.inverseScale);
        if(relativeWidthResult.first.has_value()) {
            float width = relativeWidthResult.first.value();
            if(objInfoBeingEdited)
                width *= penWidth * 0.5f;
            else
                width *= 0.5f;
            width += 1.0f;
            Vector2f pos = drawData.main->input.mouse.pos;
            SkPaint linePaint;
            linePaint.setAntiAlias(drawData.skiaAA);
            linePaint.setColor4f({1.0f, 1.0f, 1.0f, 1.0f});
            linePaint.setStyle(SkPaint::kStroke_Style);
            linePaint.setStrokeCap(SkPaint::kRound_Cap);
            linePaint.setStrokeWidth(0.0f);
            linePaint.setBlender(CanvasTheme::get_visible_blend_mode());
            SkPath circ = SkPath::Circle(pos.x(), pos.y(), width);
            canvas->drawPath(circ, linePaint);
        }
    }
}
