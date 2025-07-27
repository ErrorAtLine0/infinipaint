#pragma once
#include <execution>
#ifdef __APPLE__
    #define POOLSTL_STD_SUPPLEMENT
    #include <poolstl.hpp>
#endif

template <typename ContainerType> void parallel_loop_container(const ContainerType& c, std::function<void(const typename ContainerType::value_type&)> func) {
#ifdef __EMSCRIPTEN__
    std::for_each(c.begin(), c.end(), func);
#else
    std::for_each(std::execution::par_unseq, c.begin(), c.end(), func);
#endif
}
