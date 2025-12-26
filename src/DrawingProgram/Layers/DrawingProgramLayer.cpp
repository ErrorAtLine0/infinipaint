#include "DrawingProgramLayer.hpp"
#include "DrawingProgramLayerManager.hpp"
#include "../DrawingProgram.hpp"
#include <Helpers/Parallel.hpp>

void DrawingProgramLayer::draw(SkCanvas* canvas, const DrawData& drawData) {
    for(auto& p : components->get_data())
        p->obj->draw(canvas, drawData);
}

void DrawingProgramLayer::set_component_list_callbacks(DrawingProgramLayerManager& layerMan) {
    components->set_insert_callback([&](const CanvasComponentContainer::ObjInfoSharedPtr& c) {
        c->obj->set_owner_obj_info(c);
        c->obj->commit_update(layerMan.drawP); // Run commit update on insert so that world bounds are calculated
    });
}

void DrawingProgramLayer::commit_update_dont_invalidate_cache(DrawingProgramLayerManager& layerMan) {
    parallel_loop_container(components->get_data(), [&layerMan](auto& comp) {
        comp->obj->commit_update_dont_invalidate_cache(layerMan.drawP);
    });
}
