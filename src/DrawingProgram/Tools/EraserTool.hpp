#pragma once
#include <include/core/SkCanvas.h>
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"

class DrawingProgram;

class EraserTool : public DrawingProgramToolBase {
    public:
        EraserTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void right_click_popup_gui(Vector2f popupPos) override;
        virtual void erase_component(CanvasComponentContainer::ObjInfo* erasedComp) override;
        virtual void tool_update() override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;

        std::unordered_set<CanvasComponentContainer::ObjInfo*> erasedComponents; // Pointers will be erased from this set if theyre erased in the main list (done by callback)
    private:
        void erase_between_points(const Vector2f& start, const Vector2f& end);
        std::optional<Vector2f> lastPosOpt;
        bool isErasing = false;
};
