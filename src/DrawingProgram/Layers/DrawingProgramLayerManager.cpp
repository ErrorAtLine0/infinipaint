#include "DrawingProgramLayerManager.hpp"
#include "../../World.hpp"
#include "Helpers/NetworkingObjects/NetObjOrderedList.hpp"
#include "Helpers/Parallel.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"
#include "../../CanvasComponents/CanvasComponentAllocator.hpp"
#include "../../CanvasComponents/CanvasComponent.hpp"

DrawingProgramLayerManager::DrawingProgramLayerManager(DrawingProgram& drawProg):
    drawP(drawProg), // initialize drawP first for next objects to be initialized properly
    listGUI(*this)
{}

void DrawingProgramLayerManager::server_init_no_file() {
    layerTreeRoot = drawP.world.netObjMan.make_obj_from_ptr<DrawingProgramLayerListItem>(new DrawingProgramLayerListItem(drawP.world.netObjMan, "ROOT", true));
    layerTreeRoot->get_folder().set_component_list_callbacks(*this); // Only need to call set_component_list_callbacks on root, and the rest will get the callbacks set as well
    editingLayer = layerTreeRoot->get_folder().folderList->emplace_back_direct(layerTreeRoot->get_folder().folderList, drawP.world.netObjMan, "First Layer", false)->obj;
}

void DrawingProgramLayerManager::scale_up(const WorldScalar& scaleUpAmount) {
    layerTreeRoot->scale_up(scaleUpAmount);
}

void DrawingProgramLayerManager::write_components_server(cereal::PortableBinaryOutputArchive& a) {
    layerTreeRoot.write_create_message(a);
}

void DrawingProgramLayerManager::read_components_client(cereal::PortableBinaryInputArchive& a) {
    layerTreeRoot = drawP.world.netObjMan.read_create_message<DrawingProgramLayerListItem>(a, nullptr);
    commitUpdateOnComponentInsert = false;
    layerTreeRoot->set_component_list_callbacks(*this);
    commitUpdateOnComponentInsert = true;
    auto flattenedCompList = get_flattened_component_list();
    parallel_loop_container(flattenedCompList, [&drawP = drawP](CanvasComponentContainer::ObjInfo* comp) {
        comp->obj->commit_update_dont_invalidate_cache(drawP);
    });
    drawP.rebuild_cache();
    editingLayer = layerTreeRoot->get_folder().get_initial_editing_layer();
}

bool DrawingProgramLayerManager::is_a_layer_being_edited() {
    return !editingLayer.expired();
}

uint32_t DrawingProgramLayerManager::edited_layer_component_count() {
    return editingLayer.lock()->get_layer().components->size();
}

uint32_t DrawingProgramLayerManager::total_component_count() {
    if(layerTreeRoot)
        return layerTreeRoot->get_component_count();
    return 0;
}

CanvasComponentContainer::ObjInfoIterator DrawingProgramLayerManager::get_edited_layer_end_iterator() {
    return editingLayer.lock()->get_layer().components->end();
}

void DrawingProgramLayerManager::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(layerTreeRoot)
        layerTreeRoot->draw(canvas, drawData);
}

std::vector<CanvasComponentContainer::ObjInfo*> DrawingProgramLayerManager::get_flattened_component_list() const {
    std::vector<CanvasComponentContainer::ObjInfo*> toRet;
    layerTreeRoot->get_flattened_component_list(toRet);
    return toRet;
}

std::vector<DrawingProgramLayerListItem*> DrawingProgramLayerManager::get_flattened_layer_list() {
    std::vector<DrawingProgramLayerListItem*> toRet;
    layerTreeRoot->get_flattened_layer_list(toRet);
    return toRet;
}

bool DrawingProgramLayerManager::layer_tree_root_exists() {
    return layerTreeRoot;
}

const DrawingProgramLayerListItem& DrawingProgramLayerManager::get_layer_root() {
    return *layerTreeRoot;
}

bool DrawingProgramLayerManager::component_passes_layer_selector(CanvasComponentContainer::ObjInfo* c, LayerSelector layerSelector) {
    switch(layerSelector) {
        case LayerSelector::ALL_VISIBLE_LAYERS:
            return c->obj->parentLayer->get_visible();
        case LayerSelector::LAYER_BEING_EDITED:
            return is_a_layer_being_edited() && c->obj->parentLayer == editingLayer.lock().get();
    }
    return false;
}

std::vector<CanvasComponentContainer::ObjInfoIterator> DrawingProgramLayerManager::add_many_components_to_layer_being_edited(const std::vector<std::pair<CanvasComponentContainer::ObjInfoIterator, CanvasComponentContainer*>>& newObjs) {
    auto editLayerPtr = editingLayer.lock();
    if(!editLayerPtr)
        return {};
    return editLayerPtr->get_layer().components->insert_ordered_list_and_send_create(editLayerPtr->get_layer().components, newObjs);
}

CanvasComponentContainer::ObjInfo* DrawingProgramLayerManager::add_component_to_layer_being_edited(CanvasComponentContainer* newObj) {
    auto editLayerPtr = editingLayer.lock();
    if(!editLayerPtr)
        return nullptr;
    return &(*editLayerPtr->get_layer().components->push_back_and_send_create(editLayerPtr->get_layer().components, newObj));
}


void DrawingProgramLayerManager::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    if(version >= VersionNumber(0, 4, 0)) {
        layerTreeRoot = drawP.world.netObjMan.make_obj_from_ptr<DrawingProgramLayerListItem>(new DrawingProgramLayerListItem());
        layerTreeRoot->load_file(a, version, *this);
        commitUpdateOnComponentInsert = false;
        layerTreeRoot->get_folder().set_component_list_callbacks(*this); // Only need to call set_component_list_callbacks on root, and the rest will get the callbacks set as well
        commitUpdateOnComponentInsert = true;
    }
    else {
        server_init_no_file();
        auto editingLayerTmpPtr = editingLayer.lock();
        uint64_t compCount;
        a(compCount);
        for(uint64_t i = 0; i < compCount; i++) {
            CanvasComponentContainer* newContainer = new CanvasComponentContainer();
            newContainer->load_file(a, version, drawP.world.netObjMan);
            editingLayerTmpPtr->get_layer().components->push_back_and_send_create(editingLayerTmpPtr->get_layer().components, newContainer);
        }
    }

    auto flattenedCompList = get_flattened_component_list();
    parallel_loop_container(flattenedCompList, [&drawP = drawP](CanvasComponentContainer::ObjInfo* comp) {
        comp->obj->commit_update_dont_invalidate_cache(drawP);
    });
    drawP.rebuild_cache();
    editingLayer = layerTreeRoot->get_folder().get_initial_editing_layer();
}

void DrawingProgramLayerManager::save_file(cereal::PortableBinaryOutputArchive& a) const {
    layerTreeRoot->save_file(a);
}

void DrawingProgramLayerManager::get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) {
    layerTreeRoot->get_used_resources(resourceSet);
}
