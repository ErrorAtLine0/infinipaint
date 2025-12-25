#pragma once
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "../../CanvasComponents/CanvasComponent.hpp"

class DrawingProgramLayerListItem;

class DrawingProgramLayer {
    public:
        void draw(SkCanvas* canvas, const DrawData& drawData);

        CanvasComponentContainer::NetListOwnerPtr components;
};
