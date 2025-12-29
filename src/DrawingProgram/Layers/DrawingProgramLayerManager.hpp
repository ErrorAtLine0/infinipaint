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
        uint32_t edited_layer_component_count();
        bool component_passes_layer_selector(const CanvasComponentContainer::ObjInfoSharedPtr& c, LayerSelector layerSelector);

        bool layer_tree_root_exists();
        const DrawingProgramLayerListItem& get_layer_root();

        CanvasComponentContainer::ObjInfoSharedPtr add_component_to_layer_being_edited(CanvasComponentContainer* newObj);
        std::vector<CanvasComponentContainer::ObjInfoSharedPtr> add_many_components_to_layer_being_edited(const std::vector<std::pair<uint32_t, CanvasComponentContainer*>>& newObjs);
        std::vector<CanvasComponentContainer::ObjInfoSharedPtr> get_flattened_component_list() const;
        std::vector<DrawingProgramLayerListItem*> get_flattened_layer_list();
        DrawingProgramLayerManagerGUI listGUI;
};
