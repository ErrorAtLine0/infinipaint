#pragma once
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "../../CanvasComponents/CanvasComponent.hpp"

class DrawingProgramLayerListItem;

class DrawingProgramLayer {
    public:
        CanvasComponentContainer::NetListOwnerPtr components;
};
