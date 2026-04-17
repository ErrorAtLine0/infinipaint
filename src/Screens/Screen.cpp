#include "Screen.hpp"

Screen::Screen(MainProgram& m):
    main(m)
{}

void Screen::update() {}
void Screen::gui_layout_run() {}
bool Screen::app_close_requested() { return true; }
void Screen::input_add_file_to_canvas_callback(const CustomEvents::AddFileToCanvasEvent& addFile) {}
void Screen::input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile) {}
void Screen::input_paste_callback(const CustomEvents::PasteEvent& paste) {}
void Screen::input_drop_file_callback(const InputManager::DropCallbackArgs& drop) {}
void Screen::input_drop_text_callback(const InputManager::DropCallbackArgs& drop) {}
void Screen::input_key_callback(const InputManager::KeyCallbackArgs& key) {}
void Screen::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {}
void Screen::input_text_callback(const InputManager::TextCallbackArgs& text) {}
void Screen::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {}
void Screen::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {}
void Screen::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) {}
void Screen::input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button) {}
void Screen::input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch) {}
void Screen::input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion) {}
void Screen::input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) {}
void Screen::input_multi_finger_touch_callback(const InputManager::MultiFingerTouchCallbackArgs& touch) {}
void Screen::input_multi_finger_motion_callback(const InputManager::MultiFingerMotionCallbackArgs& motion) {}
void Screen::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {}
void Screen::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {}
void Screen::input_window_resize_callback(const InputManager::WindowResizeCallbackArgs& w) {}
void Screen::input_window_scale_callback(const InputManager::WindowScaleCallbackArgs& w) {}

Screen::~Screen() {}
