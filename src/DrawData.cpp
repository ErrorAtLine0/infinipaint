#include "DrawData.hpp"
#include "DrawComponents/DrawComponent.hpp"

void DrawData::refresh_draw_optimizing_values() {
    clampDrawBetween = true;
    clampDrawMinimum = cam.c.inverseScale >> DRAWCOMP_MIN_SHIFT_BEFORE_DISAPPEAR;
    mipMapLevelOne = cam.c.inverseScale >> DRAWCOMP_MIPMAP_LEVEL_ONE;
    mipMapLevelTwo = cam.c.inverseScale >> DRAWCOMP_MIPMAP_LEVEL_TWO;
    clampDrawMaximum = cam.c.inverseScale << DRAWCOMP_MAX_SHIFT_BEFORE_DISAPPEAR;
}
