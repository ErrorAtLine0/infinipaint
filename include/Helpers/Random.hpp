#pragma once
#include <random>

class Random {
    public:
        static Random& get();
        Random();
        template <typename T> T int_range(T start, T end) {
            std::uniform_int_distribution<T> distribute(start, end);
            return distribute(randGen);
        }
        template <typename T> T real_range(T start, T end) {
            std::uniform_real_distribution<T> distribute(start, end);
            return distribute(randGen);
        }
        std::string alphanumeric_str(size_t len);
        std::mt19937_64 randGen;
    private:
        static Random global;
};
