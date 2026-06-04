/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "../InputManager.hpp"

class Screen {
    public:
        Screen(MainProgram& m);
        virtual void update();
        virtual void draw(SkCanvas* canvas) = 0;

        struct ExtensionFilter {
            std::string name;
            std::string extensions;
        };

        typedef std::function<void(const std::filesystem::path&, const ExtensionFilter& extensionSelected)> OpenFileSelectorCallback;
        struct NativeFilePicker {
            std::atomic<bool> isOpen = false;
            std::vector<ExtensionFilter> extensionFiltersComplete;
            std::vector<SDL_DialogFileFilter> sdlFileFilters;
            OpenFileSelectorCallback postSelectionFunc;
        };
        static NativeFilePicker nativeFilePicker;

        virtual void gui_layout_run();
        virtual bool app_close_requested();
        virtual void input_add_file_to_canvas_callback(const CustomEvents::AddFileToCanvasEvent& addFile);
        virtual void input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile);
        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste);
        virtual void input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput);
        virtual void input_mobile_import_canvas_callback(const CustomEvents::MobileImportCanvasEvent& mobileImport);
        virtual void input_global_back_button_callback();
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
        virtual void input_app_about_to_go_to_background_callback();
        virtual float calculate_gui_scale();
        virtual void on_tab_close();
        virtual std::optional<InputManager::TextBoxStartInfo> get_text_box_start_info();

        virtual void open_file_selector(const std::string& filePickerName, const std::vector<ExtensionFilter>& extensionFilters, OpenFileSelectorCallback postSelectionFunc, const std::string& fileName = "", bool isSaving = false);

        virtual ~Screen();
    protected:
        static void sdl_open_file_dialog_callback(void* userData, const char * const * fileList, int filter);

        MainProgram& main;
};
