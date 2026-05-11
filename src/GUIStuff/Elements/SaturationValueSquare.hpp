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

struct SaturationValueSquareData {
    float* saturation = nullptr;
    float* value = nullptr;
    float* hue = nullptr;
    std::function<void()> onChange;
    std::function<void()> onHold;
    std::function<void()> onRelease;
};

class SaturationValueSquare : public Element {
    public:
        SaturationValueSquare(GUIManager& gui);

        void layout(const Clay_ElementId& id, const SaturationValueSquareData& opts);
        virtual void update() override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;

    private:
        void update_sv(Vector2f p);
        SaturationValueSquareData o;
        float currentHue = -1.0f;
        float currentSaturation = -1.0f;
        float currentValue = -1.0f;
        bool isHeld = false;
};

}
