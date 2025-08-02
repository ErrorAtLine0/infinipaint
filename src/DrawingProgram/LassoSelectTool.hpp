#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include <Helpers/SCollision.hpp>
#include "../CoordSpaceHelper.hpp"
#include "../DrawComponents/DrawComponent.hpp"

class DrawingProgram;

class LassoSelectTool {
    public:
        LassoSelectTool(DrawingProgram& initDrawP);
        void gui_toolbox();
        void tool_update();
        void draw(SkCanvas* canvas, const DrawData& drawData);
        void reset_tool();

        bool prevent_undo_or_redo();
    private:
        struct LassoSelectControls {
            int selectionMode = 0;

            CoordSpaceHelper coords;

            std::vector<Vector2f> lassoPoints;
        } controls;

        DrawingProgram& drawP;
};
