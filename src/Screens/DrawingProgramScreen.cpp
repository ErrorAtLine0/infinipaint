#include "DrawingProgramScreen.hpp"
#include "../MainProgram.hpp"

DrawingProgramScreen::DrawingProgramScreen(MainProgram& m):
    Screen(m),
    toolbar(m)
{}

void DrawingProgramScreen::update() {
    toolbar.update();
    main.world->focus_update();
}

void DrawingProgramScreen::gui_layout_run() {
    toolbar.layout_run();
}

bool DrawingProgramScreen::app_close_requested() {
    return toolbar.app_close_requested();
}

void DrawingProgramScreen::input_add_file_to_canvas_callback(const CustomEvents::AddFileToCanvasEvent& addFile) {
    main.world->input_add_file_to_canvas_callback(addFile);
}

void DrawingProgramScreen::input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile) {
    main.create_new_tab(openFile);
}

void DrawingProgramScreen::input_paste_callback(const CustomEvents::PasteEvent& paste) {
    main.world->input_paste_callback(paste);
}

void DrawingProgramScreen::input_drop_file_callback(const InputManager::DropCallbackArgs& drop) {
    if(std::filesystem::is_regular_file(drop.data)) {
        std::filesystem::path droppedFilePath(drop.data);
        if(droppedFilePath.has_extension() && droppedFilePath.extension().string() == std::string("." + World::FILE_EXTENSION)) {
            CustomEvents::emit_event<CustomEvents::OpenInfiniPaintFileEvent>({
                .isClient = false,
                .filePathSource = droppedFilePath
            });
            return;
        }
    }
    main.world->input_drop_file_callback(drop);
}

void DrawingProgramScreen::input_drop_text_callback(const InputManager::DropCallbackArgs& drop) {
    main.world->input_drop_text_callback(drop);
}

void DrawingProgramScreen::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    switch(key.key) {
        case InputManager::KEY_NOGUI: {
            if(key.down && !key.repeat) {
                toolbar.drawGui = !toolbar.drawGui;
                main.g.gui.set_to_layout();
            }
            break;
        }
        case InputManager::KEY_SAVE: {
            if(key.down && !key.repeat)
                toolbar.save_func();
            break;
        }
        case InputManager::KEY_SAVE_AS: {
            if(key.down && !key.repeat)
                toolbar.save_as_func();
            break;
        }
        case InputManager::KEY_OPEN_CHAT: {
            if(key.down && !key.repeat)
                toolbar.open_chatbox();
            break;
        }
    }
    main.world->input_key_callback(key);
}

void DrawingProgramScreen::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {
    switch(key.key) {
        case InputManager::KEY_GENERIC_ESCAPE: {
            if(key.down && !key.repeat)
                toolbar.close_chatbox();
            break;
        }
    }
    main.world->input_text_key_callback(key);
}

void DrawingProgramScreen::input_text_callback(const InputManager::TextCallbackArgs& text) {
    main.world->input_text_callback(text);
}

void DrawingProgramScreen::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    main.world->input_mouse_button_callback(button);
}

void DrawingProgramScreen::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    main.world->input_mouse_motion_callback(motion);
}

void DrawingProgramScreen::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) {
    main.world->input_mouse_wheel_callback(wheel);
}

void DrawingProgramScreen::input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button) {
    main.world->input_pen_button_callback(button);
}

void DrawingProgramScreen::input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch) {
    main.world->input_pen_touch_callback(touch);
}

void DrawingProgramScreen::input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion) {
    main.world->input_pen_motion_callback(motion);
}

void DrawingProgramScreen::input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) {
    main.world->input_pen_axis_callback(axis);
}

void DrawingProgramScreen::input_multi_finger_touch_callback(const InputManager::MultiFingerTouchCallbackArgs& touch) {
    main.world->input_multi_finger_touch_callback(touch);
}

void DrawingProgramScreen::input_multi_finger_motion_callback(const InputManager::MultiFingerMotionCallbackArgs& motion) {
    main.world->input_multi_finger_motion_callback(motion);
}

void DrawingProgramScreen::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
}

void DrawingProgramScreen::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {
}

void DrawingProgramScreen::input_window_resize_callback(const InputManager::WindowResizeCallbackArgs& w) {
}

void DrawingProgramScreen::input_window_scale_callback(const InputManager::WindowScaleCallbackArgs& w) {
}
