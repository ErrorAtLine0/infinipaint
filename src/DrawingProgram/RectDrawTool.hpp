#pragma once
#include "DrawingProgramToolBase.hpp"
#include "../DrawComponents/DrawRectangle.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class RectDrawTool : public DrawingProgramToolBase {
    public:
        RectDrawTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual bool right_click_popup_gui(Vector2f popupPos) override;
        virtual void tool_update() override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
    private:
        void commit_rectangle();

        struct RectDrawControls {
            Vector2f startAt;
            float relativeRadiusWidth = 10.0f;
            int fillStrokeMode = 1;
            int drawStage = 0;
            std::shared_ptr<DrawRectangle> intermediateItem;
        } controls;
};
