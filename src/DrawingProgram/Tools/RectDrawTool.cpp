#include "RectDrawTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "../../CanvasComponents/RectangleCanvasComponent.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/NumberSliderHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"

RectDrawTool::RectDrawTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType RectDrawTool::get_type() {
    return DrawingProgramToolType::RECTANGLE;
}

void RectDrawTool::gui_toolbox() {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;
    auto& toolConfig = drawP.world.main.toolConfig;
    auto& fillStrokeMode = toolConfig.rectDraw.fillStrokeMode;
    auto& relativeRadiusWidth = toolConfig.rectDraw.relativeRadiusWidth;
    gui.new_id("rect draw tool", [&] {
        text_label_centered(gui, "Draw Rectangle");
        slider_scalar_field(gui, "relradiuswidth", "Corner Radius", &relativeRadiusWidth, 0.0f, 40.0f);
        radio_button_selector(gui, "fill type", &fillStrokeMode, {
            {"Fill only", 0},
            {"Outline only", 1},
            {"Fill and Outline", 2}
        });
        if(fillStrokeMode == 1 || fillStrokeMode == 2)
            toolConfig.relative_width_gui(drawP, "Outline Size");
    });
}

void RectDrawTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        if(button.down && drawP.layerMan.is_a_layer_being_edited() && !objInfoBeingEdited && !drawP.world.main.g.gui.cursor_obstructed()) {
            auto& toolConfig = drawP.world.main.toolConfig;
            auto& fillStrokeMode = toolConfig.rectDraw.fillStrokeMode;
            auto& relativeRadiusWidth = toolConfig.rectDraw.relativeRadiusWidth;

            auto relativeWidthResult = drawP.world.main.toolConfig.get_relative_width_stroke_size(drawP, drawP.world.drawData.cam.c.inverseScale);
            auto relativeRadiusWidthResult = drawP.world.main.toolConfig.get_relative_width_from_value(drawP, drawP.world.drawData.cam.c.inverseScale, relativeRadiusWidth);
            if(!relativeWidthResult.first.has_value() || !relativeRadiusWidthResult.first.has_value()) {
                drawP.world.main.toolConfig.print_relative_width_fail_message(relativeWidthResult.second);
                return;
            }
            float width = relativeWidthResult.first.value();
            float radiusWidth = relativeRadiusWidthResult.first.value();

            CanvasComponentContainer* newContainer = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::RECTANGLE);
            RectangleCanvasComponent& newRectangle = static_cast<RectangleCanvasComponent&>(newContainer->get_comp());

            startAt = button.pos;
            newRectangle.d.strokeColor = toolConfig.globalConf.foregroundColor;
            newRectangle.d.fillColor = toolConfig.globalConf.backgroundColor;
            newRectangle.d.cornerRadius = radiusWidth;
            newRectangle.d.strokeWidth = width;
            newRectangle.d.p1 = startAt;
            newRectangle.d.p2 = startAt;
            newRectangle.d.p2 = ensure_points_have_distance(newRectangle.d.p1, newRectangle.d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);
            newRectangle.d.fillStrokeMode = static_cast<uint8_t>(fillStrokeMode);
            newContainer->coords = drawP.world.drawData.cam.c;
            objInfoBeingEdited = drawP.layerMan.add_component_to_layer_being_edited(newContainer);
        }
        else if(!button.down && objInfoBeingEdited)
            commit();
    }
}

void RectDrawTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        Vector2f newPos = containerPtr->coords.from_cam_space_to_this(drawP.world, motion.pos);
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
}

void RectDrawTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    if(objInfoBeingEdited == erasedComp)
        objInfoBeingEdited = nullptr;
}

void RectDrawTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
}

void RectDrawTool::switch_tool(DrawingProgramToolType newTool) {
    commit();
}

void RectDrawTool::tool_update() {
}

bool RectDrawTool::prevent_undo_or_redo() {
    return objInfoBeingEdited;
}

void RectDrawTool::commit() {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        containerPtr->commit_update(drawP);
        containerPtr->send_comp_update(drawP, true);
        if(containerPtr->get_world_bounds().has_value())
            drawP.layerMan.add_undo_place_component(objInfoBeingEdited);
        else {
            auto& components = containerPtr->parentLayer->get_layer().components;
            components->erase(components, containerPtr->objInfo);
        }
        objInfoBeingEdited = nullptr;
    }
}

void RectDrawTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}
