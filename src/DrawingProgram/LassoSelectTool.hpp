#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include <Helpers/SCollision.hpp>
#include "../CoordSpaceHelper.hpp"
#include "../DrawComponents/DrawComponent.hpp"
#include "DrawingProgramToolBase.hpp"

class DrawingProgram;

class LassoSelectTool : public DrawingProgramToolBase {
    public:
        LassoSelectTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void tool_update() override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual bool prevent_undo_or_redo() override;

    private:
        struct LassoSelectControls {
            int selectionMode = 0;

            CoordSpaceHelper coords;

            std::vector<Vector2f> lassoPoints;
        } controls;
};
