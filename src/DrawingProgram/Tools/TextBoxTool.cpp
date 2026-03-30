#include "TextBoxTool.hpp"
#include <chrono>
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include <cereal/types/vector.hpp>
#include <memory>
#include "../../CanvasComponents/TextBoxCanvasComponent.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"
#include "EditTool.hpp"

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"

TextBoxTool::TextBoxTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType TextBoxTool::get_type() {
    return DrawingProgramToolType::TEXTBOX;
}

void TextBoxTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.new_id("textbox tool", [&] {
        GUIStuff::ElementHelpers::text_label_centered(t.gui, "Zoom tool");
    });
}

void TextBoxTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        if(button.down && drawP.layerMan.is_a_layer_being_edited() && !objInfoBeingEdited) {
            startAt = button.pos;
            endAt = startAt;

            CanvasComponentContainer* newContainer = new CanvasComponentContainer(drawP.world.netObjMan, CanvasComponentType::TEXTBOX);
            TextBoxCanvasComponent& newTextBox = static_cast<TextBoxCanvasComponent&>(newContainer->get_comp());

            newTextBox.d.p1 = newTextBox.d.p2 = startAt;
            newTextBox.d.p2 = ensure_points_have_distance(newTextBox.d.p1, newTextBox.d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);
            newTextBox.d.editing = true;
            newContainer->coords = drawP.world.drawData.cam.c;
            objInfoBeingEdited = drawP.layerMan.add_component_to_layer_being_edited(newContainer);
        }
        else if(!button.down && objInfoBeingEdited) {
            auto editTool = std::make_unique<EditTool>(drawP);
            editTool->edit_start(objInfoBeingEdited, false);
            drawP.toolToSwitchToAfterUpdate = std::move(editTool);
        }
    }
}

void TextBoxTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        TextBoxCanvasComponent& textBox = static_cast<TextBoxCanvasComponent&>(containerPtr->get_comp());

        Vector2f newPos = containerPtr->coords.from_cam_space_to_this(drawP.world, motion.pos);
        if(drawP.world.main.input.key(InputManager::KEY_GENERIC_LSHIFT).held) {
            float height = std::fabs(startAt.y() - newPos.y());
            newPos.x() = startAt.x() + (((newPos.x() - startAt.x()) < 0.0f ? -1.0f : 1.0f) * height);
        }
        endAt = newPos;
        textBox.d.p1 = cwise_vec_min(endAt, startAt);
        textBox.d.p2 = cwise_vec_max(endAt, startAt);
        textBox.d.p2 = ensure_points_have_distance(textBox.d.p1, textBox.d.p2, MINIMUM_DISTANCE_BETWEEN_BOUNDS);

        containerPtr->send_comp_update(drawP, false);
        containerPtr->commit_update(drawP);
    }
}

void TextBoxTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
    if(objInfoBeingEdited == erasedComp)
        objInfoBeingEdited = nullptr;
}

void TextBoxTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
}

void TextBoxTool::switch_tool(DrawingProgramToolType newTool) {
    commit();
}

void TextBoxTool::tool_update() {
}

bool TextBoxTool::prevent_undo_or_redo() {
    return objInfoBeingEdited;
}

void TextBoxTool::commit() {
    if(objInfoBeingEdited) {
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentContainer>& containerPtr = objInfoBeingEdited->obj;
        containerPtr->commit_update(drawP);
        containerPtr->send_comp_update(drawP, true);
        drawP.layerMan.add_undo_place_component(objInfoBeingEdited);
        objInfoBeingEdited = nullptr;
    }
}

void TextBoxTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}
