#pragma once
#include "DrawingProgramToolBase.hpp"
#include "../DrawComponents/DrawEllipse.hpp"
#include <Helpers/SCollision.hpp>
#include <any>
#include "DrawingProgramEditToolBase.hpp"

class DrawingProgram;


class EditTool : public DrawingProgramToolBase {
    public:
        struct HandleData {
            Vector2f* p;
            Vector2f* min;
            Vector2f* max;
            float minimumDistanceBetweenBoundsAndPoint = MINIMUM_DISTANCE_BETWEEN_BOUNDS;
        };

        EditTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void tool_update() override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;

        void add_point_handle(const HandleData& handle);
        void edit_start(const std::shared_ptr<DrawComponent>& comp);
        bool is_editable(const std::shared_ptr<DrawComponent>& comp);

        struct EditControls {
            std::unique_ptr<DrawingProgramEditToolBase> compEditTool;
            std::vector<HandleData> pointHandles;
            std::weak_ptr<DrawComponent> compToEdit;
            bool isEditing = false;
            HandleData* pointDragging = nullptr;
            std::any prevData;
        } controls;
};
