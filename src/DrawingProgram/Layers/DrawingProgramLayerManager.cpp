#include "DrawingProgramLayerManager.hpp"
#include "../../World.hpp"

DrawingProgramLayerManager::DrawingProgramLayerManager(DrawingProgram& drawProg):
    drawP(drawProg), // initialize drawP first for next objects to be initialized properly
    listGUI(*this)
{}

void DrawingProgramLayerManager::init() {
    if(drawP.world.netObjMan.is_server()) {
        layerTreeRoot = drawP.world.netObjMan.make_obj_from_ptr<DrawingProgramLayerListItem>(new DrawingProgramLayerListItem(drawP.world.netObjMan, "ROOT", true));
        layerTreeRoot->get_folder().set_component_list_callbacks(*this); // Only need to call set_component_list_callbacks on root, and the rest will get the callbacks set as well
        editingLayer = layerTreeRoot->get_folder().folderList->emplace_back_direct(layerTreeRoot->get_folder().folderList, drawP.world.netObjMan, "First Layer", false)->obj;
    }
}

void DrawingProgramLayerManager::write_components_server(cereal::PortableBinaryOutputArchive& a) {
    layerTreeRoot.write_create_message(a);
}

void DrawingProgramLayerManager::read_components_client(cereal::PortableBinaryInputArchive& a) {
    layerTreeRoot = drawP.world.netObjMan.read_create_message<DrawingProgramLayerListItem>(a, nullptr);
    layerTreeRoot->commit_update_dont_invalidate_cache(*this);
    layerTreeRoot->set_component_list_callbacks(*this);
}

bool DrawingProgramLayerManager::is_a_layer_being_edited() {
    return !editingLayer.expired();
}

void DrawingProgramLayerManager::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(layerTreeRoot)
        layerTreeRoot->draw(canvas, drawData);
}
