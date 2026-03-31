#include "EllipseDrawTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "Helpers/MathExtras.hpp"
#include "../../InputManager.hpp"
#include <cereal/types/vector.hpp>
#include "../../CanvasComponents/EllipseCanvasComponent.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"

#include "../../GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"

EllipseDrawTool::EllipseDrawTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType EllipseDrawTool::get_type() {
    return DrawingProgramToolType::ELLIPSE;
}

void EllipseDrawTool::gui_toolbox() {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    Toolbar& t = drawP.world.main.toolbar;
    auto& gui = drawP.world.main.g.gui;
    auto& toolConfig = drawP.world.main.toolConfig;
    auto& fillStrokeMode = toolConfig.ellipseDraw.fillStrokeMode;
    gui.new_id("ellipse draw tool", [&] {
        text_label_centered(gui, "Draw Ellipse");
        radio_button_selector(gui, "fill type", &fillStrokeMode, {
            {"Fill only", 0},
            {"Outline only", 1},
            {"Fill and Outline", 2}
        });
        if(fillStrokeMode == 1 || fillStrokeMode == 2)
            toolConfig.relative_width_gui(drawP, "Outline Size");
    });
}

void EllipseDrawTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        if(button.down && drawP.layerMan.is_a_layer_being_edited() && !objInfoBeingEdited) {
            auto& toolConfig = drawP.world.main.toolConfig;

            auto relativeWidthResult = drawP.world.main.toolConfig.get_relative_width_stroke_size(drawP, drawP.world.drawData.cam.c.inverseScale);
            if(!relativeWidthResult.first.has_value()) {
                drawP.world.main.toolConfig.print_relative_width_fail_message(relativeWidthResult.second);
                return;
            }
            float width = relativeWidthResult.first.value();

            CanvasComponentContainer* newContainer = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::ELLIPSE);
            EllipseCanvasComponent& newEllipse = static_cast<EllipseCanvasComponent&>(newContainer->get_comp());

            startAt = button.pos;
            newEllipse.d.strokeColor = toolConfig.globalConf.foregroundColor;
            newEllipse.d.fillColor =   toolConfig.globalConf.backgroundColor;
            newEllipse.d.strokeWidth = width;
            newEllipse.d.p1 = startAt;
            newEllipse.d.p2 = startAt;
            newEllipse.d.p2 = ensure_points_have_distance(newEllipse.d.p1, newEllipse.d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);
            newEllipse.d.fillStrokeMode = static_cast<uint8_t>(toolConfig.ellipseDraw.fillStrokeMode);
            newContainer->coords = drawP.world.drawData.cam.c;

            objInfoBeingEdited = drawP.layerMan.add_component_to_layer_being_edited(newContainer);
        }
        else if(!button.down && objInfoBeingEdited)
            commit();
    }
}

void EllipseDrawTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        Vector2f newPos = containerPtr->coords.from_cam_space_to_this(drawP.world, motion.pos);
        if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held) {
            float height = std::fabs(startAt.y() - newPos.y());
            newPos.x() = startAt.x() + (((newPos.x() - startAt.x()) < 0.0f ? -1.0f : 1.0f) * height);
        }
        EllipseCanvasComponent& ellipse = static_cast<EllipseCanvasComponent&>(containerPtr->get_comp());
        ellipse.d.p1 = cwise_vec_min(startAt, newPos);
        ellipse.d.p2 = cwise_vec_max(startAt, newPos);
        ellipse.d.p2 = ensure_points_have_distance(ellipse.d.p1, ellipse.d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);
        containerPtr->send_comp_update(drawP, false);
        containerPtr->commit_update(drawP);
    }
}

void EllipseDrawTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    if(objInfoBeingEdited == erasedComp)
        objInfoBeingEdited = nullptr;
}

void EllipseDrawTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
}

void EllipseDrawTool::switch_tool(DrawingProgramToolType newTool) {
    commit();
}

void EllipseDrawTool::tool_update() {
}

bool EllipseDrawTool::prevent_undo_or_redo() {
    return objInfoBeingEdited;
}

void EllipseDrawTool::commit() {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        containerPtr->commit_update(drawP);
        containerPtr->send_comp_update(drawP, true);
        drawP.layerMan.add_undo_place_component(objInfoBeingEdited);
        objInfoBeingEdited = nullptr;
    }
}

void EllipseDrawTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}
