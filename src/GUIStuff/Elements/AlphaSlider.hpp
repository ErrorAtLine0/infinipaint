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

struct AlphaSliderData {
    float* alpha = nullptr;
    float* r = nullptr;
    float* g = nullptr;
    float* b = nullptr;
    std::function<void()> onChange;
    std::function<void()> onHold;
    std::function<void()> onRelease;
};

class AlphaSlider : public Element {
    public:
        AlphaSlider(GUIManager& gui);

        void layout(const Clay_ElementId& id, const AlphaSliderData& opts);
        virtual void update() override;
        virtual void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command, bool skiaAA) override;

    private:
        void update_alpha(Vector2f p);
        AlphaSliderData o;
        float currentAlpha = -1.0f;
        float currentR = -1.0f;
        float currentG = -1.0f;
        float currentB = -1.0f;
        bool isHeld = false;
};

}
