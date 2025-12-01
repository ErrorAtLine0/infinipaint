#pragma once
#include "DrawingProgramToolBase.hpp"
#include "../WorldGrid.hpp"

class DrawingProgram;

class GridModifyTool : public DrawingProgramToolBase {
    public:
        GridModifyTool(DrawingProgram& initDrawP);
        void set_grid_id(ServerClientID newGridID);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual bool right_click_popup_gui(Vector2f popupPos) override;
        virtual void tool_update() override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
    private:
        WorldGrid oldGrid;
        ServerClientID gridID;
        unsigned selectionMode = 0;
};
