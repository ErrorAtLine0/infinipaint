#pragma once
#include "DrawingProgramToolBase.hpp"
#include "../DrawComponents/DrawEllipse.hpp"
#include <any>
#include <Helpers/SCollision.hpp>

class DrawingProgram;

class EllipseDrawTool : public DrawingProgramToolBase {
    public:
        EllipseDrawTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void tool_update() override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
    private:
        void commit();

        struct EllipseDrawControls {
            Vector2f startAt;
            int fillStrokeMode = 1;
            int drawStage = 0;
            std::shared_ptr<DrawEllipse> intermediateItem;
        } controls;
};
