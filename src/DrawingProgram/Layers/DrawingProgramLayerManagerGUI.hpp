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
        void setup_list_gui(const char* id, bool& hoveringOverDropdown);
    private:
        std::optional<std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<DrawingProgramLayerListItem>>> try_to_create_in_proper_position(DrawingProgramLayerListItem* newItem);
        std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<DrawingProgramLayerListItem>> create_in_proper_position(DrawingProgramLayerListItem* newItem);
        NetworkingObjects::NetObjOrderedListIterator<DrawingProgramLayerListItem> create_layer(DrawingProgramLayerListItem* newItem);
        void remove_layer(const NetworkingObjects::NetObjID& parentID, const NetworkingObjects::NetObjID& objectID);
        void editing_layer_check();

        GUIStuff::TreeListing::SelectionData selectionData;
        std::unordered_set<NetworkingObjects::NetObjID> oldSelection;
        std::string nameToEdit;
        std::string nameForNew;
        float alphaValToEdit = 0.0f;
        size_t blendModeValToEdit = 0;

        std::optional<DrawingProgramLayerListItemMetaInfo> editingData;

        DrawingProgramLayerManager& layerMan;
};
