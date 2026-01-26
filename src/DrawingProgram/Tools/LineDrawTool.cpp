#include "LineDrawTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "../../CanvasComponents/BrushStrokeCanvasComponent.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"
#include "Helpers/MathExtras.hpp"
#include <limits>

LineDrawTool::LineDrawTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType LineDrawTool::get_type() {
    return DrawingProgramToolType::LINE;
}

void LineDrawTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    auto& toolConfig = drawP.world.main.toolConfig;
    t.gui.push_id("rect draw tool");
    t.gui.text_label_centered("Draw Line");
    toolConfig.relative_width_slider(t.gui, "Size", &toolConfig.lineDraw.relativeWidth);
    t.gui.checkbox_field("hasroundcaps", "Round Caps", &toolConfig.lineDraw.hasRoundCaps);
    t.gui.pop_id();
}

void LineDrawTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    if(objInfoBeingEdited == erasedComp)
        objInfoBeingEdited = nullptr;
}

bool LineDrawTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
    return true;
}

void LineDrawTool::switch_tool(DrawingProgramToolType newTool) {
    commit();
}

void LineDrawTool::tool_update() {
    auto& toolConfig = drawP.world.main.toolConfig;
    if(!objInfoBeingEdited) {
        if(drawP.controls.leftClick && drawP.layerMan.is_a_layer_being_edited()) {
            CanvasComponentContainer* newBrushStrokeContainer = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::BRUSHSTROKE);
            BrushStrokeCanvasComponent& newBrushStroke = static_cast<BrushStrokeCanvasComponent&>(newBrushStrokeContainer->get_comp());

            BrushStrokeCanvasComponentPoint p;
            p.pos = drawP.world.main.input.mouse.pos;
            p.width = toolConfig.get_relative_width(toolConfig.lineDraw.relativeWidth);
            newBrushStroke.d->points.emplace_back(p);
            p.pos = ensure_points_have_distance(p.pos, p.pos, 1.0f);
            newBrushStroke.d->points.emplace_back(p);
            newBrushStroke.d->color = toolConfig.globalConf.foregroundColor;
            newBrushStroke.d->hasRoundCaps = toolConfig.lineDraw.hasRoundCaps;
            newBrushStrokeContainer->coords = drawP.world.drawData.cam.c;
            objInfoBeingEdited = drawP.layerMan.add_component_to_layer_being_edited(newBrushStrokeContainer);
        }
    }
    else {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        constexpr float SNAP_DIVISION_COUNT = 12.0f;
        if(drawP.controls.leftClickHeld) {
            BrushStrokeCanvasComponent& brushStroke = static_cast<BrushStrokeCanvasComponent&>(containerPtr->get_comp());
            Vector2f newPos = containerPtr->coords.get_mouse_pos(drawP.world);
            Vector2f oldPos = brushStroke.d->points.front().pos;
            if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held) {
                Vector2f diff = (newPos - oldPos);
                float diffLength = diff.norm();
                diff.normalize();
                float angle = (std::atan2(diff.y(), diff.x()) / std::numbers::pi) * SNAP_DIVISION_COUNT;
                angle = std::round(angle);
                angle = (angle * std::numbers::pi) / SNAP_DIVISION_COUNT;
                newPos = oldPos + diffLength * Vector2f{cos(angle), sin(angle)};
            }
            brushStroke.d->points.back().pos = ensure_points_have_distance(oldPos, newPos, 1.0f);
            containerPtr->send_comp_update(drawP, false);
            containerPtr->commit_update(drawP);
        }
        else
            commit();
    }
}

bool LineDrawTool::prevent_undo_or_redo() {
    return objInfoBeingEdited;
}

void LineDrawTool::commit() {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        containerPtr->commit_update(drawP);
        containerPtr->send_comp_update(drawP, true);
        drawP.layerMan.add_undo_place_component(objInfoBeingEdited);
        objInfoBeingEdited = nullptr;
    }
}

void LineDrawTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}
