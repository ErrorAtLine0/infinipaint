#pragma once
#include "DrawingProgramToolBase.hpp"
#include <Helpers/SCollision.hpp>
#include <Helpers/NetworkingObjects/NetObjWeakPtr.hpp>
#include "../../CanvasComponents/CanvasComponentContainer.hpp"

class DrawingProgram;

class EllipseDrawTool : public DrawingProgramToolBase {
    public:
        EllipseDrawTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox(Toolbar& t) override;
        virtual void right_click_popup_gui(Toolbar& t, Vector2f popupPos) override;
        virtual void erase_component(CanvasComponentContainer::ObjInfo* erasedComp) override;
        virtual void tool_update() override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
    private:
        void commit();

        Vector2f startAt;
        CanvasComponentContainer::ObjInfo* objInfoBeingEdited = nullptr;
};
