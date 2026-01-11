#include "DrawingProgramLayerFolder.hpp"
#include "DrawingProgramLayerListItem.hpp"
#include "DrawingProgramLayer.hpp"
#include <Helpers/Parallel.hpp>
#include "DrawingProgramLayerManager.hpp"
#include "../DrawingProgram.hpp"
#include "../../World.hpp"

void DrawingProgramLayerFolder::draw(SkCanvas* canvas, const DrawData& drawData) const {
    for(auto& p : (*folderList) | std::views::reverse)
        p.obj->draw(canvas, drawData);
}

void DrawingProgramLayerFolder::set_component_list_callbacks(DrawingProgramLayerManager& layerMan) {
    folderList->set_insert_callback([&](auto& c) {
        layerMan.drawP.drawCache.clear_own_cached_surfaces();
        c->obj->set_component_list_callbacks(layerMan);
    });
    folderList->set_erase_callback([&](auto& c) {
        layerMan.drawP.drawCache.clear_own_cached_surfaces();
        c->obj->set_to_erase();
    });
    folderList->set_move_callback([&](auto& c, uint32_t oldPos) {
        layerMan.drawP.drawCache.clear_own_cached_surfaces();
    });
    for(auto& listItem : *folderList)
        listItem.obj->set_component_list_callbacks(layerMan);
}

void DrawingProgramLayerFolder::set_to_erase() {
    for(auto& listItem : *folderList)
        listItem.obj->set_to_erase();
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

void DrawingProgramLayerFolder::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version, DrawingProgramLayerManager& layerMan) {
    folderList = layerMan.drawP.world.netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<DrawingProgramLayerListItem>>();
    uint32_t folderListSize;
    a(folderListSize);
    for(uint32_t i = 0; i < folderListSize; i++) {
        DrawingProgramLayerListItem* item = new DrawingProgramLayerListItem();
        item->load_file(a, version, layerMan);
        folderList->push_back_and_send_create(folderList, item);
    }
}

void DrawingProgramLayerFolder::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(folderList->size());
    for(auto& listItem : *folderList)
        listItem.obj->save_file(a);
}

void DrawingProgramLayerFolder::get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const {
    for(auto& listItem : *folderList)
        listItem.obj->get_used_resources(resourceSet);
}
