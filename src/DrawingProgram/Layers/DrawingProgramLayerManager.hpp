#pragma once
#include "DrawingProgramLayerListItem.hpp"
#include "DrawingProgramLayerManagerGUI.hpp"
#include <unordered_set>

class DrawingProgramLayerManager {
    private:
        DrawingProgram& drawP;
        friend class DrawingProgramLayerManagerGUI;
        friend class DrawingProgramLayerListItem;
        friend class DrawingProgramLayer;
        friend class DrawingProgramLayerFolder;
        NetworkingObjects::NetObjOwnerPtr<DrawingProgramLayerListItem> layerTreeRoot;
        NetworkingObjects::NetObjWeakPtr<DrawingProgramLayerListItem> editingLayer;
        void set_initial_editing_layer();
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
        template <typename List> void erase_component_container(const List& compsToErase) {
            std::unordered_map<DrawingProgramLayerListItem*, std::vector<CanvasComponentContainer::ObjInfoIterator>> idsToEraseInSpecificLayers;
            for(auto& c : compsToErase)
                idsToEraseInSpecificLayers[c->obj->parentLayer].emplace_back(c->obj->objInfo);
            for(auto& [layerListItem, netObjSetToErase] : idsToEraseInSpecificLayers) {
                auto& layerComponentList = layerListItem->get_layer().components;
                layerComponentList->erase_list(layerComponentList, netObjSetToErase);
            }
        }
        uint32_t edited_layer_component_count();
        CanvasComponentContainer::ObjInfoIterator get_edited_layer_end_iterator();
        bool component_passes_layer_selector(CanvasComponentContainer::ObjInfo* c, LayerSelector layerSelector);

        bool layer_tree_root_exists();
        const DrawingProgramLayerListItem& get_layer_root();
        uint32_t total_component_count();

        CanvasComponentContainer::ObjInfo* add_component_to_layer_being_edited(CanvasComponentContainer* newObj);
        std::vector<CanvasComponentContainer::ObjInfoIterator> add_many_components_to_layer_being_edited(const std::vector<std::pair<CanvasComponentContainer::ObjInfoIterator, CanvasComponentContainer*>>& newObjs);
        std::vector<CanvasComponentContainer::ObjInfo*> get_flattened_component_list() const;
        std::vector<DrawingProgramLayerListItem*> get_flattened_layer_list();
        DrawingProgramLayerManagerGUI listGUI;
};
