#include "RectDrawTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "../CanvasComponents/RectangleCanvasComponent.hpp"
#include "../CanvasComponents/CanvasComponentContainer.hpp"

RectDrawTool::RectDrawTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType RectDrawTool::get_type() {
    return DrawingProgramToolType::RECTANGLE;
}

void RectDrawTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("rect draw tool");
    t.gui.text_label_centered("Draw Rectangle");
    t.gui.slider_scalar_field("relradiuswidth", "Corner Radius", &relativeRadiusWidth, 0.0f, 40.0f);
    if(t.gui.radio_button_field("fillonly", "Fill only", fillStrokeMode == 0)) fillStrokeMode = 0;
    if(t.gui.radio_button_field("outlineonly", "Outline only", fillStrokeMode == 1)) fillStrokeMode = 1;
    if(t.gui.radio_button_field("filloutline", "Fill and Outline", fillStrokeMode == 2)) fillStrokeMode = 2;
    if(fillStrokeMode == 1 || fillStrokeMode == 2)
        t.gui.slider_scalar_field("relstrokewidth", "Outline Size", &drawP.controls.relativeWidth, 3.0f, 40.0f);
    t.gui.pop_id();
}

void RectDrawTool::erase_component(const CanvasComponentContainer::ObjInfoSharedPtr& erasedComp) {
    if(objInfoBeingEdited == erasedComp) {
        objInfoBeingEdited = nullptr;
        drawStage = 0;
    }
}

bool RectDrawTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
    return true;
}

void RectDrawTool::switch_tool(DrawingProgramToolType newTool) {
    commit();
    drawStage = 0;
}

void RectDrawTool::tool_update() {
    switch(drawStage) {
        case 0: {
            if(drawP.controls.leftClick) {
                CanvasComponentContainer* newContainer = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::RECTANGLE);
                RectangleCanvasComponent& newRectangle = static_cast<RectangleCanvasComponent&>(newContainer->get_comp());

                startAt = drawP.world.main.input.mouse.pos;
                newRectangle.d.strokeColor = drawP.controls.foregroundColor;
                newRectangle.d.fillColor = drawP.controls.backgroundColor;
                newRectangle.d.cornerRadius = relativeRadiusWidth;
                newRectangle.d.strokeWidth = drawP.controls.relativeWidth;
                newRectangle.d.p1 = startAt;
                newRectangle.d.p2 = startAt;
                newRectangle.d.p2 = ensure_points_have_distance(newRectangle.d.p1, newRectangle.d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);
                newRectangle.d.fillStrokeMode = static_cast<uint8_t>(fillStrokeMode);
                newContainer->coords = drawP.world.drawData.cam.c;
                objInfoBeingEdited = drawP.components->push_back_and_send_create(drawP.components, newContainer);
                drawStage = 1;
            }
            break;
        }
        case 1: {
            NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
            if(drawP.controls.leftClickHeld) {
                Vector2f newPos = containerPtr->coords.get_mouse_pos(drawP.world);
                if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held) {
                    float height = std::fabs(startAt.y() - newPos.y());
                    newPos.x() = startAt.x() + (((newPos.x() - startAt.x()) < 0.0f ? -1.0f : 1.0f) * height);
                }
                RectangleCanvasComponent& rectangle = static_cast<RectangleCanvasComponent&>(containerPtr->get_comp());
                rectangle.d.p1 = cwise_vec_min(startAt, newPos);
                rectangle.d.p2 = cwise_vec_max(startAt, newPos);
                rectangle.d.p2 = ensure_points_have_distance(rectangle.d.p1, rectangle.d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);
                containerPtr->send_comp_update(drawP, false);
                containerPtr->commit_update(drawP);
            }
            else {
                commit();
                drawStage = 0;
            }
            break;
        }
    }
}

bool RectDrawTool::prevent_undo_or_redo() {
    return (drawStage == 1);
}

void RectDrawTool::commit() {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        containerPtr->commit_update(drawP);
        containerPtr->send_comp_update(drawP, true);
        objInfoBeingEdited = nullptr;
    }
}

void RectDrawTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}
