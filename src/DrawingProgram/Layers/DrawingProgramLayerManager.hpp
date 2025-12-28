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
        enum class LayerSelector {
            ALL_VISIBLE_LAYERS,
            LAYER_BEING_EDITED
        };

        DrawingProgramLayerManager(DrawingProgram& drawProg);
        void init();
        void draw(SkCanvas* canvas, const DrawData& drawData);
        void write_components_server(cereal::PortableBinaryOutputArchive& a);
        void read_components_client(cereal::PortableBinaryInputArchive& a);
        bool is_a_layer_being_edited();
        void erase_component_set(const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& compsToErase);
        bool component_passes_layer_selector(const CanvasComponentContainer::ObjInfoSharedPtr& c, LayerSelector layerSelector);

        bool layer_tree_root_exists();
        const DrawingProgramLayerListItem& get_layer_root();

        template <typename T> CanvasComponentContainer::ObjInfoSharedPtr add_component_to_layer_being_edited(T* newObj) {
            auto editLayerPtr = editingLayer.lock();
            if(!editLayerPtr)
                return nullptr;
            return editLayerPtr->get_layer().components->push_back_and_send_create(editLayerPtr->get_layer().components, newObj);
        }
        std::vector<CanvasComponentContainer::ObjInfoSharedPtr> get_flattened_component_list() const;
        DrawingProgramLayerManagerGUI listGUI;
};
