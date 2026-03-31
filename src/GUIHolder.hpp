#pragma once

#include "GUIStuff/GUIManager.hpp"

class GUIHolder {
    public:
        GUIHolder();

        GUIStuff::GUIManager gui;

        void load_icons_at(const std::filesystem::path& pathToLoad);
        void load_default_theme();
        void save_theme(const std::filesystem::path& configPath, const std::string& themeName);
        bool load_theme(const std::filesystem::path& configPath, const std::string& themeName);

        void update();
        void draw(SkCanvas* canvas);
        void input_key_callback(const InputManager::KeyCallbackArgs& key);
        void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button);
        void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion);
        void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel);
        void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch);
        void input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion);
};
