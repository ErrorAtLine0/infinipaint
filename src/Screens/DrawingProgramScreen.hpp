#pragma once
#include "Screen.hpp"
#include "../Toolbar.hpp"

class DrawingProgramScreen : public Screen {
    public:
        DrawingProgramScreen(MainProgram& m);
        virtual void update() override;
        virtual void gui_layout_run() override;
        virtual bool app_close_requested() override;
        virtual void input_add_file_to_canvas_callback(const CustomEvents::AddFileToCanvasEvent& addFile) override;
        virtual void input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile) override;
        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste) override;
        virtual void input_drop_file_callback(const InputManager::DropCallbackArgs& drop) override;
        virtual void input_drop_text_callback(const InputManager::DropCallbackArgs& drop) override;
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_text_callback(const InputManager::TextCallbackArgs& text) override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        virtual void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) override;
        virtual void input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button) override;
        virtual void input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch) override;
        virtual void input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion) override;
        virtual void input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) override;
        virtual void input_multi_finger_touch_callback(const InputManager::MultiFingerTouchCallbackArgs& touch) override;
        virtual void input_multi_finger_motion_callback(const InputManager::MultiFingerMotionCallbackArgs& motion) override;
        virtual void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) override;
        virtual void input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) override;
        virtual void input_window_resize_callback(const InputManager::WindowResizeCallbackArgs& w) override;
        virtual void input_window_scale_callback(const InputManager::WindowScaleCallbackArgs& w) override;
    private:
        Toolbar toolbar;
};
