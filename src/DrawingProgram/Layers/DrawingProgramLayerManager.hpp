#pragma once
#include "DrawingProgramLayerListItem.hpp"
#include "DrawingProgramLayerManagerGUI.hpp"

class DrawingProgramLayerManager {
    private:
        DrawingProgram& drawP;
        friend class DrawingProgramLayerManagerGUI;
        friend class DrawingProgramLayerListItem;
        friend class DrawingProgramLayer;
        friend class DrawingProgramLayerFolder;
        NetworkingObjects::NetObjOwnerPtr<DrawingProgramLayerListItem> layerTreeRoot;
        NetworkingObjects::NetObjWeakPtr<DrawingProgramLayerListItem> editingLayer;
    public:

        DrawingProgramLayerManager(DrawingProgram& drawProg);
        void init();
        void draw(SkCanvas* canvas, const DrawData& drawData);
        void write_components_server(cereal::PortableBinaryOutputArchive& a);
        void read_components_client(cereal::PortableBinaryInputArchive& a);
        bool is_a_layer_being_edited();
        template <typename T> CanvasComponentContainer::ObjInfoSharedPtr add_component_to_layer_being_edited(T* newObj) {
            auto editLayerPtr = editingLayer.lock();
            if(!editLayerPtr)
                return nullptr;
            return editLayerPtr->get_layer().components->push_back_and_send_create(editLayerPtr->get_layer().components, newObj);
        }
        DrawingProgramLayerManagerGUI listGUI;
};
