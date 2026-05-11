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
#include "Element.hpp"

namespace GUIStuff {

class LayoutElement : public Element {
    public:
        LayoutElement(GUIManager& gui);
        struct Callbacks {
            std::function<void(LayoutElement* l, const InputManager::MouseButtonCallbackArgs& button)> mouseButton;
            std::function<void(LayoutElement* l, const InputManager::MouseMotionCallbackArgs& motion)> mouseMotion;
            std::function<void(LayoutElement* l, const InputManager::MouseWheelCallbackArgs& wheel)> mouseWheel;
            std::function<void(LayoutElement* l, const InputManager::FingerTouchCallbackArgs& touch)> fingerTouch;
            std::function<void(LayoutElement* l, const InputManager::FingerMotionCallbackArgs& motion)> fingerMotion;
            std::function<void(LayoutElement* l, const InputManager::MouseButtonCallbackArgs& button)> onClick;
            std::function<void(LayoutElement* l, const InputManager::MouseMotionCallbackArgs& motion)> onMotion;
            std::function<void(LayoutElement* l, const InputManager::KeyCallbackArgs& key)> key;
        };
        void layout(const Clay_ElementId& id, const std::function<void(LayoutElement*, const Clay_ElementId&)>& layout, const Callbacks& c = {});
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        virtual void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) override;
        virtual void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) override;
        virtual void input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) override;
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key) override;
    private:
        Callbacks c;
};

}
