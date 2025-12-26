#pragma once
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include <include/core/SkCanvas.h>
#include "../../DrawData.hpp"

class DrawingProgramLayerListItem;
class DrawingProgramLayerManager;

class DrawingProgramLayerFolder {
    public:
        void draw(SkCanvas* canvas, const DrawData& drawData);
        void set_component_list_callbacks(DrawingProgramLayerManager& layerMan);
        void commit_update_dont_invalidate_cache(DrawingProgramLayerManager& layerMan);

        NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjOrderedList<DrawingProgramLayerListItem>> folderList;
        bool isFolderOpen = false;
};
