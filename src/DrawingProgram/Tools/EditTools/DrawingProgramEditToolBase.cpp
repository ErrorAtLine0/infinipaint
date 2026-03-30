#include "DrawingProgramEditToolBase.hpp"
#include "../../../MainProgram.hpp"

DrawingProgramEditToolBase::DrawingProgramEditToolBase(DrawingProgram& initDrawP):
    drawP(initDrawP)
{}

void DrawingProgramEditToolBase::right_click_popup_gui(CanvasComponentContainer::ObjInfo* comp, Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
}

void DrawingProgramEditToolBase::input_key_callback(CanvasComponentContainer::ObjInfo* comp, const InputManager::KeyCallbackArgs& key) {}
void DrawingProgramEditToolBase::input_mouse_button_on_canvas_callback(CanvasComponentContainer::ObjInfo* comp, const InputManager::MouseButtonCallbackArgs& button, bool isDraggingPoint) {}
void DrawingProgramEditToolBase::input_mouse_motion_callback(CanvasComponentContainer::ObjInfo* comp, const InputManager::MouseMotionCallbackArgs& motion, bool isDraggingPoint) {}

DrawingProgramEditToolBase::~DrawingProgramEditToolBase() {}
