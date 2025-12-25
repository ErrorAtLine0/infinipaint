#pragma once
#include <unordered_set>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "../../GUIStuff/Elements/TreeListing.hpp"
#include "DrawingProgramLayerListItem.hpp"

class World;
class DrawingProgramLayerManager;

class DrawingProgramLayerManagerGUI {
    public:
        DrawingProgramLayerManagerGUI(DrawingProgramLayerManager& layerManager);
        void refresh_gui_data();
        void setup_list_gui(const std::string& id, bool& hoveringOverDropdown);
    private:
        NetworkingObjects::NetObjOrderedListObjectInfoPtr<DrawingProgramLayerListItem> try_to_create_in_proper_position(DrawingProgramLayerListItem* newItem);
        NetworkingObjects::NetObjOrderedListObjectInfoPtr<DrawingProgramLayerListItem> create_in_proper_position(DrawingProgramLayerListItem* newItem);

        GUIStuff::TreeListing::SelectionData selectionData;
        std::unordered_set<NetworkingObjects::NetObjID> oldSelection;
        std::string nameToEdit;
        std::string nameForNew;
        float alphaValToEdit = 0.0f;
        size_t blendModeValToEdit = 0;

        DrawingProgramLayerManager& layerMan;
};
