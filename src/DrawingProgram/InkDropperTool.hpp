#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include <Helpers/SCollision.hpp>

class DrawingProgram;

class InkDropperTool {
    public:
        InkDropperTool(DrawingProgram& initDrawP);
        void gui_toolbox();
        void tool_update();
        void draw(SkCanvas* canvas, const DrawData& drawData);
    private:

        DrawingProgram& drawP;
};
