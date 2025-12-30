#include "DrawingProgramLayer.hpp"
#include "DrawingProgramLayerManager.hpp"
#include "../DrawingProgram.hpp"
#include <Helpers/Parallel.hpp>

void DrawingProgramLayer::draw(SkCanvas* canvas, const DrawData& drawData) const {
    for(auto& p : *components)
        p.obj->draw(canvas, drawData);
}

void DrawingProgramLayer::set_component_list_callbacks(DrawingProgramLayerListItem& layerListItem, DrawingProgramLayerManager& layerMan) const {
    components->set_insert_callback([&](const CanvasComponentContainer::ObjInfoIterator& c) {
        c->obj->objInfo = c;
        c->obj->parentLayer = &layerListItem;
        c->obj->commit_update(layerMan.drawP); // Run commit update on insert so that world bounds are calculated
        layerMan.drawP.drawCache.add_component(&(*c));
        if(c->obj->get_comp().get_type() == CanvasComponentType::IMAGE)
            layerMan.drawP.updateableComponents.emplace(&(*c));
    });
    components->set_erase_callback([&](const CanvasComponentContainer::ObjInfoIterator& c) {
        layerMan.drawP.selection.erase_component(&(*c));
        layerMan.drawP.drawCache.erase_component(&(*c));
        layerMan.drawP.drawTool->erase_component(&(*c));
        std::erase_if(layerMan.drawP.droppedDownloadingFiles, [&c](auto& downloadingFile) {
            return downloadingFile.comp == &(*c);
        });
        layerMan.drawP.updateableComponents.erase(&(*c));
    });
    components->set_move_callback([&](const CanvasComponentContainer::ObjInfoIterator& c, uint32_t oldPos) {
        layerMan.drawP.drawCache.invalidate_cache_at_aabb(c->obj->get_world_bounds());
    });
}

void DrawingProgramLayer::commit_update_dont_invalidate_cache(DrawingProgramLayerManager& layerMan) const {
    parallel_loop_container(*components, [&layerMan](auto& comp) {
        comp.obj->commit_update_dont_invalidate_cache(layerMan.drawP);
    });
}

void DrawingProgramLayer::get_flattened_component_list(std::vector<CanvasComponentContainer::ObjInfo*>& objList) const {
    for(auto& p : *components)
        objList.emplace_back(&p);
}
