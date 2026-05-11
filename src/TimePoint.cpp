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

#include "TimePoint.hpp"

TimePoint::TimePoint():
    tp(std::chrono::steady_clock::now()),
    timeSince(0.0f)
{}

void TimePoint::update_time_point() {
    tp = std::chrono::steady_clock::now();
}

void TimePoint::update_time_since() {
    timeSince = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - tp).count();
}

float TimePoint::get_time_since() const {
    return timeSince;
}

TimePoint::operator float() const {
    return get_time_since(); 
}

bool smooth_two_way_animation_time_check_for_change(float& timeToUpdate, float deltaTime, bool isWay1, float animDuration) {
    float newTimeToUpdate = std::clamp(isWay1 ? (timeToUpdate + deltaTime) : (timeToUpdate - deltaTime), 0.0f, animDuration);
    if(newTimeToUpdate != timeToUpdate) {
        timeToUpdate = newTimeToUpdate;
        return true;
    }
    return false;
}

float smooth_two_way_animation_time_get_lerp(float& timeToUpdate, float deltaTime, bool isWay1, float animDuration) {
    timeToUpdate = std::clamp(isWay1 ? (timeToUpdate + deltaTime) : (timeToUpdate - deltaTime), 0.0f, animDuration);
    return timeToUpdate / animDuration;
}

void smooth_two_way_animation_time(float& timeToUpdate, float deltaTime, bool isWay1, float animDuration) {
    timeToUpdate = std::clamp(isWay1 ? (timeToUpdate + deltaTime) : (timeToUpdate - deltaTime), 0.0f, animDuration);
}

WorldVec lerp_world_vec_double(const WorldVec& v1, const WorldVec& v2, double t) {
    return {FixedPoint::lerp_double(v1.x(), v2.x(), t), FixedPoint::lerp_double(v1.y(), v2.y(), t)};
}
