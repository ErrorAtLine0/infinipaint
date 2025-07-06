// The cubic bezier easing implementation is a C++ version of the following javascript code:
/**
 * https://github.com/gre/bezier-easing
 * BezierEasing - use bezier curve for transition easing function
 * by Gaëtan Renaudeau 2014 - 2015 – MIT License
 */
#include "BezierEasing.hpp"
#include <cmath>

BezierEasing BezierEasing::linear{0.0, 1.0, 0.0, 1.0};

BezierEasing::BezierEasing(Eigen::Vector4f c):
    BezierEasing(c.x(), c.y(), c.z(), c.w()) {}

BezierEasing::BezierEasing(float x1, float y1, float x2, float y2):
    mX1(x1),
    mY1(y1),
    mX2(x2),
    mY2(y2)
{
    for(size_t i = 0; i < KSPLINE_TABLE_SIZE; ++i)
        sampleValues[i] = calc_bezier(i * KSAMPLE_STEP_SIZE, mX1, mX2);
}

float BezierEasing::A(float aA1, float aA2) const {
    return 1.0 - 3.0 * aA2 + 3.0 * aA1;
}

float BezierEasing::B(float aA1, float aA2) const {
    return 3.0 * aA2 - 6.0 * aA1;
}

float BezierEasing::C(float aA1) const {
    return 3.0 * aA1;
}

float BezierEasing::calc_bezier(float aT, float aA1, float aA2) const {
    return ((A(aA1, aA2) * aT + B(aA1, aA2)) * aT + C(aA1)) * aT;
}

float BezierEasing::get_slope(float aT, float aA1, float aA2) const {
    return 3.0 * A(aA1, aA2) * aT * aT + 2.0 * B(aA1, aA2) * aT + C(aA1);
}

float BezierEasing::binary_subdivide(float aX, float aA, float aB) const {
    float currentX = 0;
    float currentT = 0;
    unsigned i = 0;
    do {
        currentT = aA + (aB - aA) / 2;
        currentX = calc_bezier(currentT, mX1, mX2) - aX;
        if(currentX > 0)
            aB = currentT;
        else
            aA = currentT;
    } while(std::abs(currentX) > SUBDIVISION_PRECISION && ++i < SUBDIVISION_MAX_ITERATIONS);
    return currentT;
}

float BezierEasing::newton_raphson_iterate(float aX, float aGuessT) const {
    for(unsigned i = 0; i < NEWTON_ITERATIONS; i++) {
        float currentSlope = get_slope(aGuessT, mX1, mX2);
        if(currentSlope == 0)
            return aGuessT;
        float currentX = calc_bezier(aGuessT, mX1, mX2) - aX;
        aGuessT -= currentX / currentSlope;
    }
    return aGuessT;
}

float BezierEasing::get_t_for_x(float aX) const {
    float intervalStart = 0.0;
    unsigned currentSample = 1;

    for(; currentSample != KSPLINE_TABLE_LAST && sampleValues[currentSample] <= aX; ++currentSample)
        intervalStart += KSAMPLE_STEP_SIZE;
    --currentSample;

    float dist = (aX - sampleValues[currentSample]) / (sampleValues[currentSample + 1] - sampleValues[currentSample]);
    float guessForT = intervalStart + dist * KSAMPLE_STEP_SIZE;
    float initialSlope = get_slope(guessForT, mX1, mX2);

    if(initialSlope >= NEWTON_MIN_SLOPE)
        return newton_raphson_iterate(aX, guessForT);
    else if(initialSlope == 0.0)
        return guessForT;
    else
        return binary_subdivide(aX, intervalStart, intervalStart + KSAMPLE_STEP_SIZE);
}

float BezierEasing::operator() (float x) const {
    if(x == 0.0 || x == 1.0 || (mX1 == mY1 && mX2 == mY2))
        return x;

    return calc_bezier(get_t_for_x(x), mY1, mY2);
}
