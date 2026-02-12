#pragma once
#include <clay.h>

#ifdef IS_CLIENT
    #include <include/core/SkPoint.h>
    template <typename S> S convert_vec2(const SkPoint& t) {
        return S{t.fX, t.fY};
    }
#endif

template <typename S, typename T> S convert_vec2(const T& t) {
    return S{t[0], t[1]};
}

template <typename S, typename T> S convert_vec2_narrow_double_to_float(const T& t) {
    return S{static_cast<float>(t[0]), static_cast<float>(t[1])};
}

template <typename S, typename T> S convert_vec3(const T& t) {
    return S{t[0], t[1], t[2]};
}

template <typename S, typename T> S convert_vec4(const T& t) {
    return S{t[0], t[1], t[2], t[3]};
}

template <typename S> S convert_vec4(const Clay_Color& t) {
    return S{t.r, t.g, t.b, t.a};
}

template <typename S> S convert_vec2(const Clay_Dimensions& t) {
    return S{t.width, t.height};
}
