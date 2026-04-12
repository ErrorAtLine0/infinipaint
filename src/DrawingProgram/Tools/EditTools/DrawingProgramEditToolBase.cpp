#include "DrawingProgramEditToolBase.hpp"
#include "../../../MainProgram.hpp"

DrawingProgramEditToolBase::DrawingProgramEditToolBase(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp):
    drawP(initDrawP),
    comp(initComp)
{}

void DrawingProgramEditToolBase::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    t.paint_popup(popupPos);
}

void DrawingProgramEditToolBase::input_paste_callback(const CustomEvents::PasteEvent& paste) {}
void DrawingProgramEditToolBase::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {}
void DrawingProgramEditToolBase::input_text_callback(const InputManager::TextCallbackArgs& text) {}
void DrawingProgramEditToolBase::input_key_callback(const InputManager::KeyCallbackArgs& key) {}
void DrawingProgramEditToolBase::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button, bool isDraggingPoint) {}
void DrawingProgramEditToolBase::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool isDraggingPoint) {}

DrawingProgramEditToolBase::~DrawingProgramEditToolBase() {}
