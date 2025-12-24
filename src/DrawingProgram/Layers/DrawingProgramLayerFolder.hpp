#pragma once
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>

class DrawingProgramLayerListItem;

class DrawingProgramLayerFolder {
    public:
        NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjOrderedList<DrawingProgramLayerListItem>> folderList;
        bool isFolderOpen = false;
};
