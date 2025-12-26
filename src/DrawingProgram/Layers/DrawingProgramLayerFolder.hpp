#pragma once
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include <include/core/SkCanvas.h>
#include "../../DrawData.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"

class DrawingProgramLayerListItem;
class DrawingProgramLayerManager;

class DrawingProgramLayerFolder {
    public:
        void draw(SkCanvas* canvas, const DrawData& drawData) const;
        void set_component_list_callbacks(DrawingProgramLayerManager& layerMan) const;
        void commit_update_dont_invalidate_cache(DrawingProgramLayerManager& layerMan) const;
        void get_flattened_component_list(std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& objList) const;

        NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjOrderedList<DrawingProgramLayerListItem>> folderList;
        bool isFolderOpen = false;
};
