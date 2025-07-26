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
        void reset_tool();
        void draw(SkCanvas* canvas, const DrawData& drawData);

        std::unordered_set<CollabListType::ObjectInfoPtr> erasedComponents;
        std::unordered_set<std::shared_ptr<DrawingProgramCacheBVHNode>> erasedBVHNodes;
    private:

        DrawingProgram& drawP;
};
