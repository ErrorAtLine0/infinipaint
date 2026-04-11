#pragma once
#include "DrawingProgramToolBase.hpp"
#include <Helpers/SCollision.hpp>
#include <any>
#include "EditTools/DrawingProgramEditToolBase.hpp"

class DrawingProgram;

class EditTool : public DrawingProgramToolBase {
    public:
        struct HandleData {
            Vector2f* p;
            Vector2f* min;
            Vector2f* max;
            float minimumDistanceBetweenMinAndPoint = MINIMUM_DISTANCE_BETWEEN_BOUNDS;
            float minimumDistanceBetweenMaxAndPoint = MINIMUM_DISTANCE_BETWEEN_BOUNDS;
            Affine2f coordMatrix = Affine2f::Identity();
        };

        EditTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void right_click_popup_gui(Vector2f popupPos) override;
        virtual void tool_update() override;
        virtual void erase_component(CanvasComponentContainer::ObjInfo* erasedComp) override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste) override;
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_text_callback(const InputManager::TextCallbackArgs& text) override;
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        ~EditTool();

        void add_point_handle(const HandleData& handle);
        void edit_start(CanvasComponentContainer::ObjInfo* comp, bool initUndoAfterEditDone = true);
        bool is_editable(CanvasComponentContainer::ObjInfo* comp);

        std::unique_ptr<DrawingProgramEditToolBase> compEditTool;
        std::vector<HandleData> pointHandles;
        CanvasComponentContainer::ObjInfo* objInfoBeingEdited = nullptr;
        HandleData* pointDragging = nullptr;
        std::any prevData;
        bool undoAfterEditDone;

        std::unique_ptr<CanvasComponent> oldData;
};
