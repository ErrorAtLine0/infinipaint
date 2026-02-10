#include "ScaleUpCanvas.hpp"

WorldScalar get_canvas_scale_up_amount(uint32_t newGridSize, uint32_t oldGridSize) {
    return FixedPoint::pow_int(CANVAS_SCALE_UP_STEP, newGridSize - oldGridSize);
}
