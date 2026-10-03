#pragma once
#include <algorithm>
#include <cmath>

namespace PenInput {
// Width mapping only: no access to previous points and no backwards smoothing.
// Windows pressure is normalized by InputManager before this stage.
inline float pressureFactor(float pressure, float minimum, bool enabled) {
    if (!enabled) return 1.0f;
    if (!std::isfinite(pressure) || pressure <= 0) return 0.0f;
    pressure = std::clamp(pressure, 0.0f, 1.0f);
    minimum = std::isfinite(minimum) ? std::clamp(minimum, 0.0f, 1.0f) : 0.0f;
    return minimum + pressure*(1.0f-minimum);
}
}
