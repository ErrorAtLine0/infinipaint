#include "DrawingProgramLayerFolder.hpp"
#include "DrawingProgramLayerListItem.hpp"
#include "DrawingProgramLayer.hpp"
#include <Helpers/Parallel.hpp>

void DrawingProgramLayerFolder::draw(SkCanvas* canvas, const DrawData& drawData) const {
    for(auto& p : (*folderList) | std::views::reverse)
        p.obj->draw(canvas, drawData);
}

void DrawingProgramLayerFolder::set_component_list_callbacks(DrawingProgramLayerManager& layerMan) {
    folderList->set_insert_callback([&](auto& c) {
        c->obj->set_component_list_callbacks(layerMan);
    });
    folderList->set_erase_callback([&](auto& c) {
        c->obj->set_to_erase();
    });
    for(auto& listItem : *folderList)
        listItem.obj->set_component_list_callbacks(layerMan);
}

void DrawingProgramLayerFolder::set_to_erase() {
    for(auto& listItem : *folderList)
        listItem.obj->set_to_erase();
}

void DrawingProgramLayerFolder::commit_update_dont_invalidate_cache(DrawingProgramLayerManager& layerMan) const {
    for(auto& listItem : *folderList)
        listItem.obj->commit_update_dont_invalidate_cache(layerMan);
}

void DrawingProgramLayerFolder::get_flattened_component_list(std::vector<CanvasComponentContainer::ObjInfo*>& objList) const {
    for(auto& listItem : (*folderList) | std::views::reverse)
        listItem.obj->get_flattened_component_list(objList);
}

NetworkingObjects::NetObjWeakPtr<DrawingProgramLayerListItem> DrawingProgramLayerFolder::get_initial_editing_layer() const {
    // BFS, select a layer that's closest to root as possible
    for(auto& c : *folderList) {
        if(!c.obj->is_folder())
            return c.obj;
    }
    for(auto& c : *folderList) {
        if(c.obj->is_folder()) {
            NetworkingObjects::NetObjWeakPtr<DrawingProgramLayerListItem> toRet = c.obj->get_folder().get_initial_editing_layer();
            if(!toRet.expired())
                return toRet;
        }
    }
    return {};
}

void DrawingProgramLayerFolder::scale_up(const WorldScalar& scaleUpAmount) {
    for(auto& p : *folderList)
        p.obj->scale_up(scaleUpAmount);
}
