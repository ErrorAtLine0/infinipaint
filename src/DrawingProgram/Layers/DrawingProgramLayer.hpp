#pragma once
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "../../CanvasComponents/CanvasComponent.hpp"

class DrawingProgramLayerListItem;
class DrawingProgramLayerManager;

class DrawingProgramLayer {
    public:
        void draw(SkCanvas* canvas, const DrawData& drawData) const;
        void set_component_list_callbacks(DrawingProgramLayerManager& layerMan) const;
        void commit_update_dont_invalidate_cache(DrawingProgramLayerManager& layerMan) const;
        void get_flattened_component_list(std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& objList) const;

        CanvasComponentContainer::NetListOwnerPtr components;
};
