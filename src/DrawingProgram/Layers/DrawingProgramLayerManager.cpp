#include "DrawingProgramLayerManager.hpp"
#include "../../World.hpp"

DrawingProgramLayerManager::DrawingProgramLayerManager(DrawingProgram& drawProg):
    drawP(drawProg), // initialize drawP first for next objects to be initialized properly
    listGUI(*this)
{}

void DrawingProgramLayerManager::init() {
    if(drawP.world.netObjMan.is_server())
        layerTreeRoot = drawP.world.netObjMan.make_obj_from_ptr<DrawingProgramLayerListItem>(new DrawingProgramLayerListItem(drawP.world.netObjMan, "ROOT", true));
}

void DrawingProgramLayerManager::write_components_server(cereal::PortableBinaryOutputArchive& a) {
    layerTreeRoot.write_create_message(a);
}

void DrawingProgramLayerManager::read_components_client(cereal::PortableBinaryInputArchive& a) {
    layerTreeRoot = drawP.world.netObjMan.read_create_message<DrawingProgramLayerListItem>(a, nullptr);
}
