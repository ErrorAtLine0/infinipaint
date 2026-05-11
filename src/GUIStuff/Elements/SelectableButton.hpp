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

class SelectableButton : public Element {
    public:
        enum class DrawType {
            TRANSPARENT_ALL,
            FILLED,
            FILLED_INVERSE,
            TRANSPARENT_BORDER
        };

        struct InnerContentCallbackParameters {
            bool isSelected;
            bool isHovering;
            bool isHeld;
        };

        struct Data {
            DrawType drawType = DrawType::TRANSPARENT_ALL;
            bool isSelected = false;
            bool instantResponse = false;
            std::function<void()> onClick;
            std::function<void(SelectableButton*)> onClickButton;
            std::function<void(const InnerContentCallbackParameters&)> innerContent;
        };

        SelectableButton(GUIManager& gui);
        void layout(const Clay_ElementId& id, const Data& d);
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        virtual void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) override;
        virtual void input_finger_motion_callback(const InputManager::FingerMotionCallbackArgs& motion) override;

    private:
        bool inDynamicArea = false;
        bool instantResponse = false;
        bool isHeld = false;
        bool isHovering = false;
        bool moved = false;
        std::function<void()> onClick;
};

}
