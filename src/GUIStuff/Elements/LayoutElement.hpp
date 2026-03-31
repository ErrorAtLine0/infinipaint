#pragma once
#include "Element.hpp"

namespace GUIStuff {

class LayoutElement : public Element {
    public:
        LayoutElement(GUIManager& gui);
        struct Callbacks {
            std::function<bool(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering)> mouseButton;
            std::function<bool(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering)> mouseMotion;
            std::function<bool(const InputManager::MouseWheelCallbackArgs& wheel, bool mouseHovering)> mouseWheel;
            std::function<void(const InputManager::KeyCallbackArgs& key)> key;
        };
        void layout(const Clay_ElementId& id, const std::function<void(const Clay_ElementId&)>& layout, const Callbacks& c = {});
        virtual bool input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button, bool mouseHovering) override;
        virtual bool input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion, bool mouseHovering) override;
        virtual bool input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel, bool mouseHovering) override;
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key) override;
    private:
        Callbacks c;
};

}
