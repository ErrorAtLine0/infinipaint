#pragma once
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include <include/core/SkCanvas.h>
#include "../../DrawData.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"

class DrawingProgramLayerListItem;
class DrawingProgramLayerManager;

struct DrawingProgramLayerFolderInitData {
    std::vector<DrawingProgramLayerFolderInitData> folderList;
};

class DrawingProgramLayerFolder {
    public:
        void draw(SkCanvas* canvas, const DrawData& drawData) const;
        void set_component_list_callbacks(DrawingProgramLayerManager& layerMan);
        void get_flattened_component_list(std::vector<CanvasComponentContainer::ObjInfo*>& objList) const;
        void set_to_erase();
        NetworkingObjects::NetObjWeakPtr<DrawingProgramLayerListItem> get_initial_editing_layer() const;
        void scale_up(const WorldScalar& scaleUpAmount);

        NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjOrderedList<DrawingProgramLayerListItem>> folderList;
        bool isFolderOpen = false;

        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version, DrawingProgramLayerManager& layerMan);
        void save_file(cereal::PortableBinaryOutputArchive& a) const;

        void get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const;
};
