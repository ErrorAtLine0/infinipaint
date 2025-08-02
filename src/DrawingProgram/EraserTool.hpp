#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include "../DrawComponents/DrawComponent.hpp"
#include "DrawingProgramToolBase.hpp"

class DrawingProgram;

class EraserTool : public DrawingProgramToolBase {
    public:
        EraserTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void tool_update() override;
        virtual void reset_tool() override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;

        std::unordered_set<CollabListType::ObjectInfoPtr> erasedComponents;
        std::unordered_set<std::shared_ptr<DrawingProgramCacheBVHNode>> erasedBVHNodes;
};
