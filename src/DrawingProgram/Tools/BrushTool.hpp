#pragma once
#include "DrawingProgramToolBase.hpp"
#include <Helpers/Hashes.hpp>
#include <include/core/SkPath.h>
#include <include/core/SkVertices.h>
#include <Helpers/SCollision.hpp>
#include <include/core/SkCanvas.h>
#include "../../CanvasComponents/CanvasComponentContainer.hpp"
#include <Helpers/NetworkingObjects/NetObjWeakPtr.hpp>

class DrawingProgram;
struct DrawData;
class BrushStrokeCanvasComponent;

class BrushTool : public DrawingProgramToolBase {
    public:
        BrushTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void tool_update() override;
        virtual void right_click_popup_gui(Vector2f popupPos) override;
        virtual void erase_component(CanvasComponentContainer::ObjInfo* erasedComp) override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        virtual void input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) override;
    private:
        bool extensive_point_checking_back(const BrushStrokeCanvasComponent& brushStroke, const Vector2f& newPoint);
        bool extensive_point_checking(const BrushStrokeCanvasComponent& brushStroke, const Vector2f& newPoint);
        void commit_stroke();

        float penWidth = 1.0f;
        bool addedTemporaryPoint = false;

        CanvasComponentContainer::ObjInfo* objInfoBeingEdited = nullptr;

        bool drawingMinimumRelativeToSize = true;
        bool midwayInterpolation = true;
        Vector2f prevPointUnaltered = {0, 0};
};
