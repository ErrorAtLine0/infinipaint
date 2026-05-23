/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "BrushTool.hpp"
#include <Helpers/ConvertVec.hpp>
#include "../../GUIStuff/GUIManager.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "DrawingProgramToolBase.hpp"
#include "../../CanvasComponents/MeshCanvasComponent.hpp"
#include "Helpers/NetworkingObjects/NetObjTemporaryPtr.decl.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/CheckBoxHelpers.hpp"

#define MINIMUM_DISTANCE_TO_NEXT_POINT 0.002f
#define DRAW_MINIMUM_LIMIT 1.0f

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
    if(objInfoBeingEdited == erasedComp) {
        objInfoBeingEdited = nullptr;
        commitUpdate = false;
    }
}

void BrushTool::smooth_out_points(float smoothFactor) {
    if(objInfoBeingEdited) {
        if(brushPoints.size() >= 2) {
            brushPoints.back().width = std::max(brushPoints[brushPoints.size() - 1].width, brushPoints[brushPoints.size() - 2].width * smoothFactor);
            for(size_t i = brushPoints.size() - 1; i > 0; i--) {
                if(brushPoints[i].width * smoothFactor > brushPoints[i - 1].width)
                    brushPoints[i - 1].width = brushPoints[i].width * smoothFactor;
                else
                    break;
            }
        }
    }
}

void BrushTool::fix_tip() {
    if(objInfoBeingEdited) {
        if(brushPoints.size() >= 2)
            brushPoints[brushPoints.size() - 2].width = brushPoints[brushPoints.size() - 1].width = std::max(brushPoints[brushPoints.size() - 1].width, brushPoints[brushPoints.size() - 2].width);
    }
}

void BrushTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        auto& toolConfig = drawP.world.main.toolConfig;
        if(button.down && drawP.layerMan.is_a_layer_being_edited() && !objInfoBeingEdited && !drawP.world.main.g.gui.cursor_obstructed()) {
            if(button.deviceType == InputManager::MouseDeviceType::PEN && drawP.world.main.conf.tabletOptions.pressureAffectsBrushWidth) {
                penWidth = drawP.world.main.input.pen.pressure;
                if(penWidth != 0.0f) {
                    float brushMinSize = drawP.world.main.conf.tabletOptions.brushMinimumSize;
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

            CanvasComponentContainer* newMeshContainer = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::MESH);
            MeshCanvasComponent& newMesh = static_cast<MeshCanvasComponent&>(newMeshContainer->get_comp());

            brushPoints.clear();
            BrushComponentCode::BrushPoint p;
            p.pos = button.pos;
            p.width = width;
            prevPointUnaltered = p.pos;
            brushPoints.emplace_back(p);
            newMesh.d.color = toolConfig.globalConf.foregroundColor;
            newMeshContainer->coords = drawP.world.drawData.cam.c;
            objInfoBeingEdited = drawP.layerMan.add_component_to_layer_being_edited(newMeshContainer);
            commit_data(false);
            addedTemporaryPoint = false;
        }
        else if(!button.down && objInfoBeingEdited)
            commit_stroke();
    }
}

void BrushTool::commit_data(bool final) {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        MeshCanvasComponent& newMesh = static_cast<MeshCanvasComponent&>(containerPtr->get_comp());
        newMesh.d.points = BrushComponentCode::brush_stroke_to_mesh_points(brushPoints, drawP.world.main.toolConfig.brush.hasRoundCaps);
        containerPtr->commit_update(drawP);
        containerPtr->send_comp_update(drawP, final);
    }
}

void BrushTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(objInfoBeingEdited) {
        auto& toolConfig = drawP.world.main.toolConfig;
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        float width = toolConfig.get_relative_width_stroke_size(drawP, containerPtr->coords.inverseScale).first.value() * penWidth;

        BrushComponentCode::BrushPoint p;
        p.pos = containerPtr->coords.from_cam_space_to_this(drawP.world, motion.pos);
        p.width = width;

        // Temporary point is a point that follows the cursor until it is placed
        if(!addedTemporaryPoint) {
            // Temporary point is close enough to be placed
            if(extensive_point_checking(p.pos)) {
                brushPoints.emplace_back(p);
                addedTemporaryPoint = true;
            }
            else
                // Temporary point not close enough yet, so change the width of the previous point based on the current pen width until it can be placed
                brushPoints.back().width = std::max(brushPoints.back().width, p.width);
        }

        if(addedTemporaryPoint) {
            const BrushComponentCode::BrushPoint& prevP = brushPoints[brushPoints.size() - 2];
            float distToPrev = (p.pos - prevP.pos).norm();
            // If the cursor isnt too close to the previous points, move the temporary point to the cursor and change the width of the temporary point to the current pen width
            if(extensive_point_checking_back(p.pos)) {
                brushPoints.back().pos = p.pos;
                brushPoints.back().width = std::max(brushPoints.back().width, p.width);
            }

            float maxWidthToCompareTo = std::max(brushPoints.back().width, p.width);
            maxWidthToCompareTo = std::max(maxWidthToCompareTo, 5.0f);
            if(brushPoints.size() >= 2)
                maxWidthToCompareTo = std::max(brushPoints[brushPoints.size() - 2].width, maxWidthToCompareTo);
            if(brushPoints.size() >= 3)
                maxWidthToCompareTo = std::max(brushPoints[brushPoints.size() - 3].width, maxWidthToCompareTo);

            // If the temporary point (cursor) is far enough from the point before it, make the temporary point permanent
            if(distToPrev >= maxWidthToCompareTo * DRAW_MINIMUM_LIMIT) {
                brushPoints.back().pos = p.pos;
                brushPoints.back().width = std::max(brushPoints.back().width, p.width);
                addedTemporaryPoint = false;

                // Midway interpolation moves the point before the last one to a position that leads to a smoother line
                if(brushPoints.size() != 2) // Don't interpolate the first point
                    brushPoints[brushPoints.size() - 2].pos = (prevPointUnaltered + p.pos) * 0.5;
                prevPointUnaltered = p.pos;
            }
        }

        smooth_out_points(drawP.world.main.conf.tabletOptions.brushPressureSmoothingFactor);
        commitUpdate = true;
    }
}

void BrushTool::input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) {
    if(axis.axis == SDL_PEN_AXIS_PRESSURE && drawP.world.main.conf.tabletOptions.pressureAffectsBrushWidth) {
        penWidth = axis.value;
        if(penWidth != 0.0f) {
            float brushMinSize = drawP.world.main.conf.tabletOptions.brushMinimumSize;
            penWidth = brushMinSize + penWidth * (1.0f - brushMinSize);
            if(objInfoBeingEdited) {
                auto& toolConfig = drawP.world.main.toolConfig;
                NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
                float width = toolConfig.get_relative_width_stroke_size(drawP, containerPtr->coords.inverseScale).first.value() * penWidth;
                brushPoints.back().width = std::max(brushPoints.back().width, width);
                smooth_out_points(drawP.world.main.conf.tabletOptions.brushPressureSmoothingFactor);
                commitUpdate = true;
            }
        }
    }
}

bool BrushTool::extensive_point_checking(const Vector2f& newPoint) {
    auto& points = brushPoints;
    if(points.size() >= 1 && (newPoint - points[points.size() - 1].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    if(points.size() >= 2 && (newPoint - points[points.size() - 2].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    if(points.size() >= 3 && (newPoint - points[points.size() - 3].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    return true;
}

bool BrushTool::extensive_point_checking_back(const Vector2f& newPoint) {
    auto& points = brushPoints;
    if(points.size() >= 2 && (newPoint - points[points.size() - 2].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    if(points.size() >= 3 && (newPoint - points[points.size() - 3].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    if(points.size() >= 4 && (newPoint - points[points.size() - 4].pos).norm() < MINIMUM_DISTANCE_TO_NEXT_POINT)
        return false;
    return true;
}

void BrushTool::tool_update() {
    if(!drawP.world.main.g.gui.cursor_obstructed())
        drawP.world.main.input.hideCursor = true;

    if(commitUpdate && objInfoBeingEdited) {
        commit_data(false);
        commitUpdate = false;
    }
}

void BrushTool::commit_stroke() {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        fix_tip();
        commit_data(true);
        commitUpdate = false;
        if(containerPtr->get_world_bounds().has_value())
            drawP.layerMan.add_undo_place_component(objInfoBeingEdited);
        else {
            auto& components = containerPtr->parentLayer->get_layer().components;
            components->erase(components, containerPtr->objInfo);
        }
        objInfoBeingEdited = nullptr;
    }
}

void BrushTool::gui_toolbox(Toolbar& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;

    gui.new_id("brush tool", [&] {
        text_label_centered(gui, "Brush");
        checkbox_boolean_field(gui, "hasroundcaps", "Round Caps", &drawP.world.main.toolConfig.brush.hasRoundCaps);
        drawP.world.main.toolConfig.relative_width_gui(drawP, "Size");
    });
}

void BrushTool::gui_phone_toolbox(PhoneDrawingProgramScreen& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;

    gui.new_id("brush tool", [&] {
        checkbox_boolean_field(gui, "hasroundcaps", "Round Caps", &drawP.world.main.toolConfig.brush.hasRoundCaps);
        drawP.world.main.toolConfig.relative_width_gui(drawP, "Size");
    });
}

void BrushTool::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    t.paint_popup(popupPos);
}

bool BrushTool::prevent_undo_or_redo() {
    return objInfoBeingEdited;
}

void BrushTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(!drawP.world.main.input.isTouchDevice && !drawData.main->g.gui.cursor_obstructed()) {
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
            SkPath circ = SkPath::Circle(pos.x(), pos.y(), width);
            canvas->drawPath(circ, linePaint);
            linePaint.setColor4f({0.0f, 0.0f, 0.0f, 1.0f});
            circ = SkPath::Circle(pos.x(), pos.y(), width - 1.0f);
            canvas->drawPath(circ, linePaint);
        }
    }
}
