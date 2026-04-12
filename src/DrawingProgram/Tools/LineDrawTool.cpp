#include "LineDrawTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "../../CanvasComponents/BrushStrokeCanvasComponent.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"
#include "Helpers/MathExtras.hpp"

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/CheckBoxHelpers.hpp"

LineDrawTool::LineDrawTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType LineDrawTool::get_type() {
    return DrawingProgramToolType::LINE;
}

void LineDrawTool::gui_toolbox(Toolbar& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;
    auto& gui = drawP.world.main.g.gui;
    auto& toolConfig = drawP.world.main.toolConfig;
    gui.new_id("rect draw tool", [&] {
        text_label_centered(gui, "Draw Line");
        toolConfig.relative_width_gui(drawP, "Size");
        checkbox_boolean_field(gui, "hasroundcaps", "Round Caps", &toolConfig.lineDraw.hasRoundCaps);
    });
}

void LineDrawTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        auto& toolConfig = drawP.world.main.toolConfig;
        if(button.down && drawP.layerMan.is_a_layer_being_edited() && !objInfoBeingEdited && !drawP.world.main.g.gui.cursor_obstructed()) {
            auto relativeWidthResult = drawP.world.main.toolConfig.get_relative_width_stroke_size(drawP, drawP.world.drawData.cam.c.inverseScale);
            if(!relativeWidthResult.first.has_value()) {
                drawP.world.main.toolConfig.print_relative_width_fail_message(relativeWidthResult.second);
                return;
            }
            float width = relativeWidthResult.first.value();

            CanvasComponentContainer* newBrushStrokeContainer = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::BRUSHSTROKE);
            BrushStrokeCanvasComponent& newBrushStroke = static_cast<BrushStrokeCanvasComponent&>(newBrushStrokeContainer->get_comp());

            BrushStrokeCanvasComponentPoint p;
            p.pos = drawP.world.main.input.mouse.pos;
            p.width = width;
            newBrushStroke.d.points->emplace_back(p);
            p.pos = ensure_points_have_distance(p.pos, p.pos, 1.0f);
            newBrushStroke.d.points->emplace_back(p);
            newBrushStroke.d.color = toolConfig.globalConf.foregroundColor;
            newBrushStroke.d.hasRoundCaps = toolConfig.lineDraw.hasRoundCaps;
            newBrushStrokeContainer->coords = drawP.world.drawData.cam.c;
            objInfoBeingEdited = drawP.layerMan.add_component_to_layer_being_edited(newBrushStrokeContainer);
        }
        else if(!button.down && objInfoBeingEdited)
            commit();
    }
}

void LineDrawTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(objInfoBeingEdited) {
        constexpr float SNAP_DIVISION_COUNT = 12.0f;
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        BrushStrokeCanvasComponent& brushStroke = static_cast<BrushStrokeCanvasComponent&>(containerPtr->get_comp());
        Vector2f newPos = containerPtr->coords.from_cam_space_to_this(drawP.world, motion.pos);
        Vector2f oldPos = brushStroke.d.points->front().pos;
        if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held) {
            Vector2f diff = (newPos - oldPos);
            float diffLength = diff.norm();
            diff.normalize();
            float angle = (std::atan2(diff.y(), diff.x()) / std::numbers::pi) * SNAP_DIVISION_COUNT;
            angle = std::round(angle);
            angle = (angle * std::numbers::pi) / SNAP_DIVISION_COUNT;
            newPos = oldPos + diffLength * Vector2f{cos(angle), sin(angle)};
        }
        brushStroke.d.points->back().pos = ensure_points_have_distance(oldPos, newPos, 1.0f);
        containerPtr->send_comp_update(drawP, false);
        containerPtr->commit_update(drawP);
    }
}

void LineDrawTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    if(objInfoBeingEdited == erasedComp)
        objInfoBeingEdited = nullptr;
}

void LineDrawTool::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    t.paint_popup(popupPos);
}

void LineDrawTool::switch_tool(DrawingProgramToolType newTool) {
    commit();
}

void LineDrawTool::tool_update() {
}

bool LineDrawTool::prevent_undo_or_redo() {
    return objInfoBeingEdited;
}

void LineDrawTool::commit() {
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

void LineDrawTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}
