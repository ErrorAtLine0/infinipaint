#pragma once
#include "DrawingProgramToolBase.hpp"
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
        virtual bool right_click_popup_gui(Vector2f popupPos) override;
        virtual void tool_update() override;
        virtual void erase_component(const CanvasComponentContainer::ObjInfoSharedPtr& erasedComp) override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;

        void add_point_handle(const HandleData& handle);
        void edit_start(const CanvasComponentContainer::ObjInfoSharedPtr& comp);
        bool is_editable(const CanvasComponentContainer::ObjInfoSharedPtr& comp);

        std::unique_ptr<DrawingProgramEditToolBase> compEditTool;
        std::vector<HandleData> pointHandles;
        CanvasComponentContainer::ObjInfoSharedPtr objInfoBeingEdited;
        bool isEditing = false;
        HandleData* pointDragging = nullptr;
        std::any prevData;
};
