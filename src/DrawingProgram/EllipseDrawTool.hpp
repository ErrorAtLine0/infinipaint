#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include "../DrawComponents/DrawEllipse.hpp"
#include <any>
#include <Helpers/SCollision.hpp>

class DrawingProgram;

class EllipseDrawTool {
    public:
        EllipseDrawTool(DrawingProgram& initDrawP);
        void gui_toolbox();
        void tool_update();
        void reset_tool();
        void draw(SkCanvas* canvas, const DrawData& drawData);

        void edit_start(const std::shared_ptr<DrawEllipse>& a, std::any& prevData);
        void commit_edit_updates(const std::shared_ptr<DrawEllipse>& a, std::any& prevData);
        bool edit_update(const std::shared_ptr<DrawEllipse>& a);
        bool edit_gui(const std::shared_ptr<DrawEllipse>& a);

        bool prevent_undo_or_redo();
    private:
        void commit();

        struct EllipseDrawControls {
            Vector2f startAt;
            int fillStrokeMode = 1;
            int drawStage = 0;
            std::shared_ptr<DrawEllipse> intermediateItem;
        } controls;

        DrawingProgram& drawP;
};
