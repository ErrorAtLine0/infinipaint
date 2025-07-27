#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include "../DrawComponents/DrawRectangle.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class RectDrawTool {
    public:
        RectDrawTool(DrawingProgram& initDrawP);
        void gui_toolbox();
        void tool_update();
        void reset_tool();
        void draw(SkCanvas* canvas, const DrawData& drawData);

        void edit_start(const std::shared_ptr<DrawRectangle>& a, std::any& prevData);
        void commit_edit_updates(const std::shared_ptr<DrawRectangle>& a, std::any& prevData);
        bool edit_update(const std::shared_ptr<DrawRectangle>& a);
        bool edit_gui(const std::shared_ptr<DrawRectangle>& a);

        bool prevent_undo_or_redo();
    private:
        void commit_rectangle();

        struct RectDrawControls {
            Vector2f startAt;
            float relativeRadiusWidth = 10.0f;
            int fillStrokeMode = 1;
            int drawStage = 0;
            std::shared_ptr<DrawRectangle> intermediateItem;
        } controls;

        DrawingProgram& drawP;
};
