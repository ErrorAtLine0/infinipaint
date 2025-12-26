#pragma once
#include "DrawingProgramToolBase.hpp"
#include "../../WorldGrid.hpp"
#include "Helpers/NetworkingObjects/NetObjWeakPtr.hpp"

class DrawingProgram;

class GridModifyTool : public DrawingProgramToolBase {
    public:
        GridModifyTool(DrawingProgram& initDrawP);
        void set_grid(const NetworkingObjects::NetObjWeakPtr<WorldGrid>& newGrid);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual bool right_click_popup_gui(Vector2f popupPos) override;
        virtual void tool_update() override;
        virtual void erase_component(const CanvasComponentContainer::ObjInfoSharedPtr& erasedComp) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
    private:
        WorldGrid oldGrid;
        NetworkingObjects::NetObjWeakPtr<WorldGrid> grid;
        unsigned selectionMode = 0;
};
