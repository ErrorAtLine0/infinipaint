#include "DrawingProgramLayerFolder.hpp"
#include "DrawingProgramLayerListItem.hpp"
#include "DrawingProgramLayer.hpp"
#include <Helpers/Parallel.hpp>

void DrawingProgramLayerFolder::draw(SkCanvas* canvas, const DrawData& drawData) const {
    for(auto& p : folderList->get_data() | std::views::reverse)
        p->obj->draw(canvas, drawData);
}

void DrawingProgramLayerFolder::set_component_list_callbacks(DrawingProgramLayerManager& layerMan) const {
    folderList->set_insert_callback([&](auto& c) {
        c->obj->set_component_list_callbacks(layerMan);
    });
    for(auto& listItem : folderList->get_data())
        listItem->obj->set_component_list_callbacks(layerMan);
}

void DrawingProgramLayerFolder::commit_update_dont_invalidate_cache(DrawingProgramLayerManager& layerMan) const {
    for(auto& listItem : folderList->get_data())
        listItem->obj->commit_update_dont_invalidate_cache(layerMan);
}

void DrawingProgramLayerFolder::get_flattened_component_list(std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& objList) const {
    for(auto& listItem : folderList->get_data())
        listItem->obj->get_flattened_component_list(objList);
}
