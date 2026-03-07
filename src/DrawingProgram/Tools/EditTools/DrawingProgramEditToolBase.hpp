#pragma once
#include <include/core/SkCanvas.h>
#include "../../../DrawData.hpp"
#include "../../../CanvasComponents/CanvasComponentContainer.hpp"

class DrawingProgram;
class EditTool;

class DrawingProgramEditToolBase {
    public:
        DrawingProgramEditToolBase(DrawingProgram& initDrawP);
        virtual void edit_start(EditTool& editTool, CanvasComponentContainer::ObjInfo* compContainer, std::any& prevData) = 0;
        virtual void commit_edit_updates(CanvasComponentContainer::ObjInfo* compContainer, std::any& prevData) = 0;
        virtual bool edit_update(CanvasComponentContainer::ObjInfo* compContainer) = 0;
        virtual bool edit_gui(CanvasComponentContainer::ObjInfo* compContainer) = 0;
        virtual bool right_click_popup_gui(CanvasComponentContainer::ObjInfo* comp, Vector2f popupPos);
        virtual void input_key_callback(CanvasComponentContainer::ObjInfo* comp, const InputManager::KeyCallbackArgs& key);
        virtual void input_mouse_button_on_canvas_callback(CanvasComponentContainer::ObjInfo* comp, const InputManager::MouseButtonCallbackArgs& button, bool isDraggingPoint);
        virtual void input_mouse_motion_callback(CanvasComponentContainer::ObjInfo* comp, const InputManager::MouseMotionCallbackArgs& motion, bool isDraggingPoint);
        virtual ~DrawingProgramEditToolBase(); 
    protected:
        DrawingProgram& drawP;
};
