#/*  
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
#include <unordered_set>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "../../GUIStuff/Elements/TreeListing.hpp"
#include "DrawingProgramLayerListItem.hpp"
#include "SerializedBlendMode.hpp"

class World;
class DrawingProgramLayerManager;

class DrawingProgramLayerManagerGUI {
    public:
        DrawingProgramLayerManagerGUI(DrawingProgramLayerManager& layerManager);
        void refresh_gui_data();
        void setup_list_gui();
    private:
        std::optional<std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<DrawingProgramLayerListItem>>> try_to_create_in_proper_position(DrawingProgramLayerListItem* newItem);
        std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<DrawingProgramLayerListItem>> create_in_proper_position(DrawingProgramLayerListItem* newItem);
        NetworkingObjects::NetObjOrderedListIterator<DrawingProgramLayerListItem> create_layer(DrawingProgramLayerListItem* newItem);
        void remove_layer(const GUIStuff::TreeListingObjIndexList& objIndex);
        void editing_layer_check();

        NetworkingObjects::NetObjTemporaryPtr<DrawingProgramLayerListItem> get_layer_parent_from_obj_index(const GUIStuff::TreeListingObjIndexList& objIndex);
        NetworkingObjects::NetObjTemporaryPtr<DrawingProgramLayerListItem> get_layer_from_obj_index(const GUIStuff::TreeListingObjIndexList& objIndex);

        std::string nameToEdit;
        std::string nameForNew;
        float alphaValToEdit = 0.0f;
        size_t blendModeValToEdit = 0;

        std::set<GUIStuff::TreeListingObjIndexList> selectedLayerIndices;
        NetworkingObjects::NetObjWeakPtr<DrawingProgramLayerListItem> editingLayer;

        std::optional<DrawingProgramLayerListItemMetaInfo> editingLayerOldMetainfo;

        DrawingProgramLayerManager& layerMan;
};
