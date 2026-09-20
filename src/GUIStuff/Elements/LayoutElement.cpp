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

#include "LayoutElement.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIManager.hpp"

namespace GUIStuff {

LayoutElement::LayoutElement(GUIManager& gui): Element(gui) {}

void LayoutElement::layout(const Clay_ElementId& id, const std::function<void(LayoutElement*, const Clay_ElementId&)>& layout, const Callbacks& c) {
    this->c = c;
    layout(this, id);
}

void LayoutElement::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(c.mouseButton) c.mouseButton(this, button);
    if(c.onClick) c.onClick(this, button);
}

void LayoutElement::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    if(c.mouseMotion) c.mouseMotion(this, motion);
    if(c.onMotion) c.onMotion(this, motion);
}

void LayoutElement::input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel) {
    if(c.mouseWheel) c.mouseWheel(this, wheel);
}

void LayoutElement::input_finger_touch_callback(const FingerInput::TouchCallbackArgs& touch) {
    if(c.fingerTouch) c.fingerTouch(this, touch);
    switch(touch.action.type) {
        case FingerInput::ActionType::MOVE: {
            InputManager::MouseMotionCallbackArgs motionArgs;
            motionArgs.deviceType = InputManager::MouseDeviceType::TOUCH;
            motionArgs.move = touch.action.motion;
            motionArgs.pos = touch.action.pos;
            if(c.onMotion) c.onMotion(this, motionArgs);
            break;
        }
        case FingerInput::ActionType::UP: {
            InputManager::MouseButtonCallbackArgs mouseArgs;
            mouseArgs.deviceType = InputManager::MouseDeviceType::TOUCH;
            mouseArgs.pos = touch.action.pos;
            mouseArgs.down = false;
            mouseArgs.clicks = 0;
            mouseArgs.button = InputManager::MouseButton::LEFT;
            if(c.onClick) c.onClick(this, mouseArgs);
            break;
        }
        case FingerInput::ActionType::DOWN: {
            InputManager::MouseButtonCallbackArgs mouseArgs;
            mouseArgs.deviceType = InputManager::MouseDeviceType::TOUCH;
            mouseArgs.pos = touch.action.pos;
            mouseArgs.down = true;
            mouseArgs.clicks = 1;
            mouseArgs.button = InputManager::MouseButton::LEFT;
            if(c.onClick) c.onClick(this, mouseArgs);
            break;
        }
        case FingerInput::ActionType::NONE:
            break;
    }
}

void LayoutElement::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    if(c.key) c.key(this, key);
}

}
