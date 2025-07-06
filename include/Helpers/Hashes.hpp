#pragma once
#include <utility>
#include <Eigen/Dense>
#include "Serializers.hpp"

using namespace Eigen;

typedef Vector<uint32_t, 2> Vector2ui32;

// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x/54728293#54728293

template <typename T, typename... Rest>
void hash_combine(uint64_t& seed, const T& v, const Rest&... rest) {
    seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hash_combine(seed, rest), ...);
}

template <> struct std::hash<Vector2i> {
    size_t operator()(const Vector2i& v) const
    {
        uint64_t h = 0;
        hash_combine(h, v.x(), v.y());
        return h;
    }
};

template <> struct std::hash<Vector2ui32> {
    size_t operator()(const Vector2ui32& v) const
    {
        uint64_t h = 0;
        hash_combine(h, v.x(), v.y());
        return h;
    }
};

template <> struct std::hash<Vector2f> {
    size_t operator()(const Vector2f& v) const
    {
        uint64_t h = 0;
        hash_combine(h, v.x(), v.y());
        return h;
    }
};

template <> struct std::hash<std::array<Vector2f, 2>> {
    size_t operator()(const std::array<Vector2f, 2>& v) const
    {
        uint64_t h = 0;
        hash_combine(h, v[0], v[1]);
        return h;
    }
};

template <> struct std::hash<std::array<float, 4>> {
    size_t operator()(const std::array<float, 4>& v) const
    {
        uint64_t h = 0;
        hash_combine(h, v[0], v[1], v[2], v[3]);
        return h;
    }
};


bool operator <(const Vector2f& a, const Vector2f& b);
