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
    private:
        std::unordered_set<CollabList<std::shared_ptr<DrawComponent>, ServerClientID>::ObjectInfoPtr> erasedComponents;
        std::unordered_set<std::shared_ptr<DrawingProgramCache::BVHNode>> erasedBVHNodes;

        void move_erased_components_from_bvh_nodes_recursive(const std::shared_ptr<DrawingProgramCache::BVHNode>& bvhNode);

        DrawingProgram& drawP;
};
