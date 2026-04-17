#pragma once
#include "../InputManager.hpp"

class Screen {
    public:
        Screen(MainProgram& m);
        virtual void update();
        virtual void draw(SkCanvas* canvas) = 0;

        virtual void gui_layout_run();
        virtual bool app_close_requested();
        virtual void input_add_file_to_canvas_callback(const CustomEvents::AddFileToCanvasEvent& addFile);
        virtual void input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile);
        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste);
        virtual void input_drop_file_callback(const InputManager::DropCallbackArgs& drop);
        virtual void input_drop_text_callback(const InputManager::DropCallbackArgs& drop);
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key);
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key);
        virtual void input_text_callback(const InputManager::TextCallbackArgs& text);
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button);
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion);
        virtual void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel);
        virtual void input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button);
        virtual void input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch);
        virtual void input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion);
        virtual void input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis);
        virtual void input_multi_finger_touch_callback(const InputManager::MultiFingerTouchCallbackArgs& touch);
        virtual void input_multi_finger_motion_callback(const InputManager::MultiFingerMotionCallbackArgs& motion);
        virtual void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch);
        virtual void input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion);
        virtual void input_window_resize_callback(const InputManager::WindowResizeCallbackArgs& w);
        virtual void input_window_scale_callback(const InputManager::WindowScaleCallbackArgs& w);

        virtual ~Screen();
    protected:
        MainProgram& main;
};
