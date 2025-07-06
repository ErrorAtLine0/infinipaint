// The cubic bezier easing implementation is a C++ version of the following javascript code:
/**
 * https://github.com/gre/bezier-easing
 * BezierEasing - use bezier curve for transition easing function
 * by Gaëtan Renaudeau 2014 - 2015 – MIT License
 */

#pragma once
#include <array>
#include <Eigen/Dense>

class BezierEasing {
    public:
        BezierEasing(Eigen::Vector4f c);
        BezierEasing(float x1, float y1, float x2, float y2);
        float operator() (float x) const;
        static BezierEasing linear;
    private:
        static constexpr unsigned NEWTON_ITERATIONS = 4;
        static constexpr float NEWTON_MIN_SLOPE = 0.001;
        static constexpr float SUBDIVISION_PRECISION = 0.0000001;
        static constexpr unsigned SUBDIVISION_MAX_ITERATIONS = 10;
        static constexpr unsigned KSPLINE_TABLE_SIZE = 11;
        static constexpr unsigned KSPLINE_TABLE_LAST = KSPLINE_TABLE_SIZE - 1;
        static constexpr float KSAMPLE_STEP_SIZE = 1.0 / (KSPLINE_TABLE_SIZE - 1.0);

        float A(float aA1, float aA2) const;
        float B(float aA1, float aA2) const;
        float C(float aA1) const;
        float calc_bezier(float aT, float aA1, float aA2) const;
        float get_slope(float aT, float aA1, float aA2) const;
        float binary_subdivide(float aX, float aA, float aB) const;
        float newton_raphson_iterate(float aX, float aGuessT) const;
        float get_t_for_x(float aX) const;

        float mX1;
        float mY1;
        float mX2;
        float mY2;

        std::array<float, KSPLINE_TABLE_SIZE> sampleValues;
};
