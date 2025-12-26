#pragma once
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "../../CanvasComponents/CanvasComponent.hpp"

class DrawingProgramLayerListItem;
class DrawingProgramLayerManager;

class DrawingProgramLayer {
    public:
        void draw(SkCanvas* canvas, const DrawData& drawData);
        void set_component_list_callbacks(DrawingProgramLayerManager& layerMan);
        void commit_update_dont_invalidate_cache(DrawingProgramLayerManager& layerMan);

        CanvasComponentContainer::NetListOwnerPtr components;
};
