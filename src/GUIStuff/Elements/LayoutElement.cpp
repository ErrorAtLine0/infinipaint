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

void LayoutElement::input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) {
    if(c.fingerTouch) c.fingerTouch(this, touch);
    if(c.onClick) {
        c.onClick(this, {
            .deviceType = InputManager::MouseDeviceType::TOUCH,
            .button = InputManager::MouseButton::LEFT,
            .down = touch.down,
            .clicks = static_cast<uint8_t>(touch.fingerTapCount),
            .pos = touch.pos
        });
    }
}

void LayoutElement::input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) {
    if(c.fingerMotion) c.fingerMotion(this, motion);
    if(c.onMotion) {
        c.onMotion(this, {
            .deviceType = InputManager::MouseDeviceType::TOUCH,
            .pos = motion.pos,
            .move = motion.move
        });
    }
}

void LayoutElement::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    if(c.key) c.key(this, key);
}

}
