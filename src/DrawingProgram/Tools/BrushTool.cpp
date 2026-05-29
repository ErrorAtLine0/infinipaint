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
#include <include/pathops/SkPathOps.h>

#define MINIMUM_DISTANCE_FROM_FIRST_POINT 3.0f
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

void BrushTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        auto& toolConfig = drawP.world.main.toolConfig;
        if(button.down && drawP.layerMan.is_a_layer_being_edited() && !objInfoBeingEdited && !drawP.world.main.g.gui.cursor_obstructed()) {
            auto relativeWidthResult = drawP.world.main.toolConfig.get_relative_width_stroke_size(drawP, drawP.world.drawData.cam.c.inverseScale);
            if(!relativeWidthResult.first.has_value()) {
                drawP.world.main.toolConfig.print_relative_width_fail_message(relativeWidthResult.second);
                return;
            }

            CanvasComponentContainer* newMeshContainer = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::MESH);
            MeshCanvasComponent& newMesh = static_cast<MeshCanvasComponent&>(newMeshContainer->get_comp());

            newMesh.d.color = toolConfig.globalConf.foregroundColor;
            newMeshContainer->coords = drawP.world.drawData.cam.c;

            BrushComponentCode::mouse_button(drawP, genData, newMeshContainer->coords, button, relativeWidthResult.first.value());

            objInfoBeingEdited = drawP.layerMan.add_component_to_layer_being_edited(newMeshContainer);
            commit_data(false);
        }
        else if(!button.down && objInfoBeingEdited)
            commit_stroke();
    }
}

void BrushTool::commit_data(bool final) {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        MeshCanvasComponent& newMesh = static_cast<MeshCanvasComponent&>(containerPtr->get_comp());
        newMesh.d.meshPath = BrushComponentCode::brush_stroke_to_skpath(genData.brushPoints, drawP.world.main.toolConfig.brush.hasRoundCaps);
        if(final) {
            containerPtr->get_comp().simplify_paths();
            containerPtr->normalize_object_coordinates();
        }
        containerPtr->commit_update(drawP);
        drawP.world.netObjMan.send_multi_update_messsage([&]() {
            drawP.send_transforms_for({objInfoBeingEdited});
            containerPtr->send_comp_update(drawP, final);
        }, NetworkingObjects::NetObjManager::SendUpdateType::SEND_TO_ALL, nullptr);
    }
    commitUpdate = false;
}

void BrushTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(objInfoBeingEdited) {
        auto& toolConfig = drawP.world.main.toolConfig;
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        BrushComponentCode::mouse_motion(drawP, genData, motion.pos, toolConfig.get_relative_width_stroke_size(drawP, containerPtr->coords.inverseScale).first.value());
        commitUpdate = true;
    }
}

void BrushTool::input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) {
    if(axis.axis == SDL_PEN_AXIS_PRESSURE && drawP.world.main.conf.tabletOptions.pressureAffectsBrushWidth) {
        genData.penWidth = axis.value;
        if(genData.penWidth != 0.0f && objInfoBeingEdited) {
            auto& toolConfig = drawP.world.main.toolConfig;
            NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
            float width = toolConfig.get_relative_width_stroke_size(drawP, containerPtr->coords.inverseScale).first.value();
            BrushComponentCode::pen_pressure(drawP, genData, width);
            commitUpdate = true;
        }
    }
}

void BrushTool::tool_update() {
    if(!drawP.world.main.g.gui.cursor_obstructed())
        drawP.world.main.input.hideCursor = true;

    if(commitUpdate && objInfoBeingEdited)
        commit_data(false);
}

void BrushTool::commit_stroke() {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        BrushComponentCode::fix_tip(genData.brushPoints);
        commit_data(true);
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
                width *= genData.penWidth * 0.5f;
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
