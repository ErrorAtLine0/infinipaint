#pragma once
#include <chrono>
#include <algorithm>
#include <cmath>
#include <Helpers/BezierEasing.hpp>
#include <include/core/SkColor.h>
#include "SharedTypes.hpp"

template <typename T> T lerp_time(T t, T tEnd, T tStart = 0) {
    return std::clamp<T>((t - tStart) / (tEnd - tStart), 0, 1);
}

WorldVec lerp_world_vec_double(const WorldVec& v1, const WorldVec& v2, double t);

template<typename T, typename TimeType> std::enable_if_t<!std::is_arithmetic_v<T>, T> lerp_vec(const T& v1, const T& v2, TimeType t) {
    T item;
    for(unsigned i = 0; i < v1.size(); i++)
        item[i] = std::lerp(v1[i], v2[i], t);
    return item;
}

template<typename T, typename TimeType> std::enable_if_t<std::is_arithmetic_v<T>, T> lerp_vec(T v1, T v2, TimeType t) {
    return std::lerp(v1, v2, t);
}

template<typename TimeType> SkColor4f lerp_vec(const SkColor4f& t1, const SkColor4f& t2, TimeType t) {
    return {std::lerp(t1[0], t2[0], t),
            std::lerp(t1[1], t2[1], t),
            std::lerp(t1[2], t2[2], t),
            std::lerp(t1[3], t2[3], t)};
}

float smooth_two_way_time(float& timeToUpdate, float deltaTime, bool isWay1, float animDuration);

class TimePoint {
    public:
        TimePoint();
        void update_time_point();
        void update_time_since();
        float get_time_since() const;
        operator float() const;
    private:
        std::chrono::time_point<std::chrono::steady_clock> tp;
        float timeSince;
};
