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

#include "Screen.hpp"
#include "../MainProgram.hpp"

Screen::Screen(MainProgram& m):
    main(m)
{}

Screen::NativeFilePicker Screen::nativeFilePicker;

// You can't trust that filter wont be -1 on the platform youre on, so dont use the extension from the callback
void Screen::sdl_open_file_dialog_callback(void* userData, const char * const * fileList, int filter) {
    if(!fileList) {
        nativeFilePicker.isOpen = false;
        return;
    }
    if(!(*fileList)) {
        nativeFilePicker.isOpen = false;
        return;
    }
    ExtensionFilter e;
    if(filter >= 0)
        e = nativeFilePicker.extensionFiltersComplete[filter];
    nativeFilePicker.postSelectionFunc(fileList[0], e);
    nativeFilePicker.isOpen = false;
}

void Screen::open_file_selector(const std::string& filePickerName, const std::vector<ExtensionFilter>& extensionFilters, OpenFileSelectorCallback postSelectionFunc, const std::string& fileName, bool isSaving) {
    if(!nativeFilePicker.isOpen) {
        nativeFilePicker.postSelectionFunc = postSelectionFunc;
        nativeFilePicker.extensionFiltersComplete = extensionFilters;
        nativeFilePicker.sdlFileFilters.clear();
        for(auto& e : nativeFilePicker.extensionFiltersComplete)
            nativeFilePicker.sdlFileFilters.emplace_back(e.name.c_str(), e.extensions.c_str());
        if(isSaving)
            SDL_ShowSaveFileDialog(sdl_open_file_dialog_callback, nullptr, main.window.sdlWindow, nativeFilePicker.sdlFileFilters.data(), nativeFilePicker.sdlFileFilters.size(), nullptr);
        else
            SDL_ShowOpenFileDialog(sdl_open_file_dialog_callback, nullptr, main.window.sdlWindow, nativeFilePicker.sdlFileFilters.data(), nativeFilePicker.sdlFileFilters.size(), nullptr, false);
    }
}

void Screen::update() {}
void Screen::gui_layout_run() {}
bool Screen::app_close_requested() { return true; }
void Screen::input_add_file_to_canvas_callback(const CustomEvents::AddFileToCanvasEvent& addFile) {}
void Screen::input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile) {}
void Screen::input_mobile_import_canvas_callback(const CustomEvents::MobileImportCanvasEvent& mobileImport) {}
void Screen::input_paste_callback(const CustomEvents::PasteEvent& paste) {}
void Screen::input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput) {}
void Screen::input_global_back_button_callback() {}
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
void Screen::input_app_about_to_go_to_background_callback() {}
void Screen::on_tab_close() {}
float Screen::calculate_gui_scale() { return main.g.final_gui_scale_not_fit(); }
std::optional<InputManager::TextBoxStartInfo> Screen::get_text_box_start_info() { return std::nullopt; }

Screen::~Screen() {}
