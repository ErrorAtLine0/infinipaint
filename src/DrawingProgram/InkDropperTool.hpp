#pragma once
#include "DrawingProgramToolBase.hpp"

class DrawingProgram;

class InkDropperTool : public DrawingProgramToolBase {
    public:
        InkDropperTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void tool_update() override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
        virtual void reset_tool() override;
};
