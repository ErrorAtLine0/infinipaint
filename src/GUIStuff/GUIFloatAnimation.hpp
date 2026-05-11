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
#include <Helpers/BezierEasing.hpp>

namespace GUIStuff {

class GUIManager;

struct GUIFloatAnimationData {
    float startVal = 0.0f;
    float endVal = 1.0f;
    float duration = 1.0f;
    BezierEasing easing = BezierEasing::linear;
};

class GUIFloatAnimation {
    public:
        GUIFloatAnimation(const GUIFloatAnimationData& animData);
        void update(GUIManager& gui);
        void animation_trigger();
        void animation_trigger_reverse();
        float get_val() const;

        bool is_at_start();
        bool is_at_end();
        bool is_done();

        bool isUsedThisFrame;
    private:
        void calculate_anim_val();
        enum class State {
            RUN,
            RUN_REVERSE,
            DONE
        } state = State::DONE;
        float currentTime = 0.0f;
        GUIFloatAnimationData anim;
        float animVal;
};

}
