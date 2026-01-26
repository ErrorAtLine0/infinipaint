#pragma once
#include "DrawingProgramToolBase.hpp"
#include <Helpers/SCollision.hpp>
#include "../../CanvasComponents/CanvasComponentContainer.hpp"

class DrawingProgram;

class LineDrawTool : public DrawingProgramToolBase {
    public:
        LineDrawTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual bool right_click_popup_gui(Vector2f popupPos) override;
        virtual void erase_component(CanvasComponentContainer::ObjInfo* erasedComp) override;
        virtual void tool_update() override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
    private:
        void commit();

        Vector2f startAt;
        CanvasComponentContainer::ObjInfo* objInfoBeingEdited = nullptr;
};
