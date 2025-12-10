#pragma once
#include "DrawingProgramToolBase.hpp"
#include "../WorldGrid.hpp"
#include "Helpers/NetworkingObjects/NetObjPtr.hpp"

class DrawingProgram;

class GridModifyTool : public DrawingProgramToolBase {
    public:
        GridModifyTool(DrawingProgram& initDrawP);
        void set_grid(const NetworkingObjects::NetObjPtr<WorldGrid>& newGrid);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual bool right_click_popup_gui(Vector2f popupPos) override;
        virtual void tool_update() override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
    private:
        WorldGrid oldGrid;
        NetworkingObjects::NetObjPtr<WorldGrid> grid;
        unsigned selectionMode = 0;
};
