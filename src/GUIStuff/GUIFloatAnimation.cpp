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

#include "GUIFloatAnimation.hpp"
#include "GUIManager.hpp"

namespace GUIStuff {

GUIFloatAnimation::GUIFloatAnimation(const GUIFloatAnimationData& animData):
    anim(animData),
    animVal(animData.startVal)
{}

bool GUIFloatAnimation::is_at_start() {
    return state == State::DONE && currentTime == 0.0f;
}

bool GUIFloatAnimation::is_at_end() {
    return state == State::DONE && currentTime == anim.duration;
}

bool GUIFloatAnimation::is_done() {
    return state == State::DONE;
}

void GUIFloatAnimation::calculate_anim_val() {
    animVal = std::lerp(anim.startVal, anim.endVal, anim.easing(currentTime / anim.duration));
}

void GUIFloatAnimation::update(GUIManager& gui) {
    switch(state) {
        case State::RUN: {
            currentTime += gui.io.deltaTime;
            if(currentTime >= anim.duration) {
                currentTime = anim.duration;
                state = State::DONE;
            }
            calculate_anim_val();
            gui.set_to_layout();
            break;
        }
        case State::RUN_REVERSE: {
            currentTime -= gui.io.deltaTime;
            if(currentTime <= 0.0f) {
                currentTime = 0.0f;
                state = State::DONE;
            }
            calculate_anim_val();
            gui.set_to_layout();
            break;
        }
        case State::DONE:
            break;
    }
}

float GUIFloatAnimation::get_val() const {
    return animVal;
}

void GUIFloatAnimation::animation_trigger() {
    if(currentTime < anim.duration)
        state = State::RUN;
}

void GUIFloatAnimation::animation_trigger_reverse() {
    if(currentTime > 0.0f)
        state = State::RUN_REVERSE;
}

}
