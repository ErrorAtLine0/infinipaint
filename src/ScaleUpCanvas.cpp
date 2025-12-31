#include "ScaleUpCanvas.hpp"

WorldScalar get_canvas_scale_up_amount(uint32_t newGridSize, uint32_t oldGridSize) {
    return WorldScalar(newGridSize - oldGridSize) * CANVAS_SCALE_UP_STEP;
}
