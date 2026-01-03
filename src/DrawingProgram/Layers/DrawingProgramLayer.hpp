#pragma once
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "../../CanvasComponents/CanvasComponent.hpp"
#include <vector>

class DrawingProgramLayerListItem;
class DrawingProgramLayerManager;

struct DrawingProgramComponentUndoData {
    WorldUndoManager::UndoObjectID undoID;
    std::unique_ptr<CanvasComponentContainer::CopyData> copyData;
};

class DrawingProgramLayer {
    public:
        void draw(SkCanvas* canvas, const DrawData& drawData) const;
        void set_component_list_callbacks(DrawingProgramLayerListItem& layerListItem, DrawingProgramLayerManager& layerMan);
        void get_flattened_component_list(std::vector<CanvasComponentContainer::ObjInfo*>& objList) const;
        void set_to_erase();
        void scale_up(const WorldScalar& scaleUpAmount);

        CanvasComponentContainer::NetListOwnerPtr components;

        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version, DrawingProgramLayerManager& layerMan);
        void save_file(cereal::PortableBinaryOutputArchive& a) const;

        void get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const;
    private:
        std::function<void(const CanvasComponentContainer::ObjInfoIterator& c)> eraseCallback;
};
