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

class CheckBox : public Element {
    public:
        CheckBox(GUIManager& gui);

        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_finger_touch_callback(const InputManager::FingerTouchCallbackArgs& touch) override;
        virtual void update() override;

        void layout(const Clay_ElementId& id, const std::function<bool()>& isTicked, const std::function<void()>& onClick);
    private:
        static constexpr float CHECKBOX_ANIMATION_TIME = 0.3;
        float hoverAnimation = 0.0;
        bool oldIsTicked = false;
        bool isHeld = false;
        bool is_hovering_animation();
        std::function<bool()> isTicked;
        std::function<void()> onClick;
};

}
