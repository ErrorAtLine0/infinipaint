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

class RotateWheel : public Element {
    public:
        RotateWheel(GUIManager& gui);
        void layout(const Clay_ElementId& id, double* rotateAngle, const std::function<void()>& onChange);
        virtual void update() override;
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;

    private:
        float wheel_start();
        float wheel_end();
        void draw_rotate_wheel(SkCanvas* canvas, UpdateInputData& io, bool skiaAA);

        double* rotateAngle = nullptr;
        std::function<void()> onChange;

        struct DisplayData {
            double rotateAngleDisplayed = 0.0;
            bool isHeld = false;
            bool isRotateBarHeld = false;
            bool isRotateBarHovered = false;
            bool operator!=(const DisplayData&) const = default;
            bool operator==(const DisplayData&) const = default;
        };

        DisplayData dd;
        DisplayData oldDD;

        void update_rotate_wheel_mouse_hover(const Vector2f& p);
        void update_rotate_wheel_mouse(const Vector2f& p);
};

}
