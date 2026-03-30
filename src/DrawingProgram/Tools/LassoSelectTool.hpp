#pragma once
#include <include/core/SkCanvas.h>
#include "../../DrawData.hpp"
#include <Helpers/SCollision.hpp>
#include "../../CoordSpaceHelper.hpp"
#include "DrawingProgramToolBase.hpp"

class DrawingProgram;

class LassoSelectTool : public DrawingProgramToolBase {
    public:
        LassoSelectTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void right_click_popup_gui(Vector2f popupPos) override;
        virtual void erase_component(CanvasComponentContainer::ObjInfo* erasedComp) override;
        virtual void tool_update() override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual bool prevent_undo_or_redo() override;
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
    private:
        struct LassoSelectControls {
            bool isSelecting = false;
            CoordSpaceHelper coords;
            std::vector<Vector2f> lassoPoints;
        } controls;
};
