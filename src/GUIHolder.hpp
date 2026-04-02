#pragma once

#include "GUIStuff/GUIManager.hpp"

class MainProgram;

class GUIHolder {
    public:
        GUIHolder(MainProgram& m);

        GUIStuff::GUIManager gui;

        void load_icons_at(const std::filesystem::path& pathToLoad);
        void load_default_theme();
        void save_theme(const std::filesystem::path& configPath, const std::string& themeName);
        bool load_theme(const std::filesystem::path& configPath, const std::string& themeName);

        void update();
        void draw(SkCanvas* canvas, bool skiaAA);
        void input_key_callback(const InputManager::KeyCallbackArgs& key);
        void input_text_key_callback();
        void input_text_input_callback();
        void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button);
        void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion);
        void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel);
        void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch);
        void input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion);

        float final_gui_scale();
    private:
        void calculate_final_gui_scale();
        float final_gui_scale_not_fit();

        float finalCalculatedGuiScale;

        MainProgram& main;
};
