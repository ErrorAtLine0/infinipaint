#include "DrawingProgramLayer.hpp"
#include "DrawingProgramLayerManager.hpp"
#include "../DrawingProgram.hpp"
#include "../../World.hpp"
#include <Helpers/Parallel.hpp>

void DrawingProgramLayer::draw(SkCanvas* canvas, const DrawData& drawData) const {
    for(auto& p : *components)
        p.obj->draw(canvas, drawData);
}

void DrawingProgramLayer::set_component_list_callbacks(DrawingProgramLayerListItem& layerListItem, DrawingProgramLayerManager& layerMan) {
    auto insertCallback = [&](const CanvasComponentContainer::ObjInfoIterator& c) {
        c->obj->objInfo = c;
        c->obj->parentLayer = &layerListItem;
        if(layerMan.commitUpdateOnComponentInsert)
            c->obj->commit_update(layerMan.drawP); // Run commit update on insert so that world bounds are calculated
        if(layerMan.addToCacheOnComponentInsert)
            layerMan.drawP.drawCache.add_component(&(*c));
        if(c->obj->get_comp().get_type() == CanvasComponentType::IMAGE)
            layerMan.drawP.updateableComponents.emplace(&(*c));
    };
    eraseCallback = [&](const CanvasComponentContainer::ObjInfoIterator& c) {
        layerMan.drawP.selection.erase_component(&(*c));
        layerMan.drawP.drawCache.erase_component(&(*c));
        layerMan.drawP.drawTool->erase_component(&(*c));
        std::erase_if(layerMan.drawP.droppedDownloadingFiles, [&c](auto& downloadingFile) {
            return downloadingFile.comp == &(*c);
        });
        layerMan.drawP.updateableComponents.erase(&(*c));
    };
    components->set_insert_callback(insertCallback);
    components->set_erase_callback(eraseCallback);
    components->set_move_callback([&](const CanvasComponentContainer::ObjInfoIterator& c, uint32_t oldPos) {
        layerMan.drawP.drawCache.invalidate_cache_at_aabb(c->obj->get_world_bounds());
    });
    // Make sure the insert callback is called on existing objects
    for(auto it = components->begin(); it != components->end(); ++it)
        insertCallback(it);
}

void DrawingProgramLayer::set_to_erase() {
    for(auto it = components->begin(); it != components->end(); ++it)
        eraseCallback(it);
}

void DrawingProgramLayer::get_flattened_component_list(std::vector<CanvasComponentContainer::ObjInfo*>& objList) const {
    for(auto& p : *components)
        objList.emplace_back(&p);
}

void DrawingProgramLayer::scale_up(const WorldScalar& scaleUpAmount) {
    for(auto& p : *components)
        p.obj->scale_up(scaleUpAmount);
}

void DrawingProgramLayer::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version, DrawingProgramLayerManager& layerMan) {
    components = layerMan.drawP.world.netObjMan.make_obj<CanvasComponentContainer::NetList>();
    uint32_t layerListSize;
    a(layerListSize);
    for(uint32_t i = 0; i < layerListSize; i++) {
        CanvasComponentContainer* item = new CanvasComponentContainer();
        item->load_file(a, version, layerMan.drawP.world.netObjMan);
        components->push_back_and_send_create(components, item);
    }
}

void DrawingProgramLayer::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(components->size());
    for(auto& comp : *components)
        comp.obj->save_file(a);
}

void DrawingProgramLayer::get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const {
    for(auto& comp : *components)
        comp.obj->get_comp().get_used_resources(resourceSet);
}
