#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include "../DrawComponents/DrawComponent.hpp"

class DrawingProgram;

class EraserTool {
    public:
        EraserTool(DrawingProgram& initDrawP);
        void gui_toolbox();
        void tool_update();
        void draw(SkCanvas* canvas, const DrawData& drawData);
    private:

        std::vector<std::pair<uint64_t, std::shared_ptr<DrawComponent>>> erasedComponents; // <placement order, object>

        DrawingProgram& drawP;
};
