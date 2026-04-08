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

bool smooth_two_way_time(float& timeToUpdate, float deltaTime, bool isWay1, float animDuration) {
    float newTimeToUpdate = std::clamp(isWay1 ? (timeToUpdate + deltaTime) : (timeToUpdate - deltaTime), 0.0f, animDuration);
    if(newTimeToUpdate != timeToUpdate) {
        timeToUpdate = newTimeToUpdate;
        return true;
    }
    return false;
}

WorldVec lerp_world_vec_double(const WorldVec& v1, const WorldVec& v2, double t) {
    return {FixedPoint::lerp_double(v1.x(), v2.x(), t), FixedPoint::lerp_double(v1.y(), v2.y(), t)};
}
