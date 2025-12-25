#include "DrawingProgramLayerFolder.hpp"
#include "DrawingProgramLayerListItem.hpp"

void DrawingProgramLayerFolder::draw(SkCanvas* canvas, const DrawData& drawData) {
    for(auto& p : folderList->get_data())
        p->obj->draw(canvas, drawData);
}
