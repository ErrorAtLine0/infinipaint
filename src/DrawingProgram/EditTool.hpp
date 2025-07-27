#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include "../DrawComponents/DrawEllipse.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class EditTool {
    public:
        struct HandleData {
            Vector2f* p;
            Vector2f* min;
            Vector2f* max;
        };

        EditTool(DrawingProgram& initDrawP);
        void gui_toolbox();
        void tool_update();
        void reset_tool();
        void draw(SkCanvas* canvas, const DrawData& drawData);
        void add_point_handle(const HandleData& handle);
        void edit_start(const std::shared_ptr<DrawComponent>& comp);
        bool prevent_undo_or_redo();

        struct EditControls {
            std::vector<HandleData> pointHandles;
            std::weak_ptr<DrawComponent> compToEdit;
            bool isEditing = false;
            HandleData* pointDragging = nullptr;
            std::any prevData;
        } controls;
    private:

        DrawingProgram& drawP;
};
