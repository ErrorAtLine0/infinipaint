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
#include <include/core/SkCanvas.h>
#include "../../DrawData.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"

class DrawingProgramLayerListItem;
class DrawingProgramLayerManager;

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
        void erase_invalid_components();
};
