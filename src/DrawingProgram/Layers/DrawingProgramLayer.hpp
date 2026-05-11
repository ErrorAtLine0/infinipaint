/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
        void erase_invalid_components();
    private:
        std::function<void(const CanvasComponentContainer::ObjInfoIterator& c)> eraseCallback;
};
