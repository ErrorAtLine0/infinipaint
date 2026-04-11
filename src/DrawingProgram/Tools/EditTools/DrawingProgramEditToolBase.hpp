#pragma once
#include <include/core/SkCanvas.h>
#include "../../../DrawData.hpp"
#include "../../../CanvasComponents/CanvasComponentContainer.hpp"

class DrawingProgram;
class EditTool;

class DrawingProgramEditToolBase {
    public:
        DrawingProgramEditToolBase(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp);
        virtual void edit_start(EditTool& editTool, std::any& prevData) = 0;
        virtual void commit_edit_updates(std::any& prevData) = 0;
        virtual bool edit_update() = 0;
        virtual void edit_gui() = 0;
        virtual void right_click_popup_gui(Vector2f popupPos);
        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste);
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key);
        virtual void input_text_callback(const InputManager::TextCallbackArgs& text);
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key);
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button, bool isDraggingPoint);
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool isDraggingPoint);
        virtual ~DrawingProgramEditToolBase(); 
    protected:
        DrawingProgram& drawP;
        CanvasComponentContainer::ObjInfo* comp;
};
