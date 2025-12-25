#include "DrawingProgramLayer.hpp"

void DrawingProgramLayer::draw(SkCanvas* canvas, const DrawData& drawData) {
    for(auto& p : components->get_data())
        p->obj->draw(canvas, drawData);
}
