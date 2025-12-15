#include "DrawData.hpp"
#include "CanvasComponents/CanvasComponentContainer.hpp"

void DrawData::refresh_draw_optimizing_values() {
    clampDrawBetween = true;
    clampDrawMinimum = cam.c.inverseScale >> CanvasComponentContainer::COMP_MIN_SHIFT_BEFORE_DISAPPEAR;
    mipMapLevelOne = cam.c.inverseScale >> CanvasComponentContainer::COMP_MIPMAP_LEVEL_ONE;
    mipMapLevelTwo = cam.c.inverseScale >> CanvasComponentContainer::COMP_MIPMAP_LEVEL_TWO;
}
