#pragma once
#include "Element.hpp"
#include <Helpers/NetworkingObjects/NetObjID.hpp>

namespace GUIStuff {

class GUIManager;

class TreeListing : public Element {
    public:
        static constexpr float ENTRY_HEIGHT = 25.0f;

        struct ParentObjectIDPair {
            NetworkingObjects::NetObjID parent;
            NetworkingObjects::NetObjID object;
            bool operator==(const ParentObjectIDPair&) const = default;
        };

        struct ParentObjectIDStack {
            std::vector<NetworkingObjects::NetObjID> parents;
            NetworkingObjects::NetObjID object;
        };

        struct DisplayData {
            struct ObjInList {
                NetworkingObjects::NetObjID id;
                bool isDirectory = false;
                bool isDirectoryOpen = false;
            };
            std::function<std::optional<ObjInList>(NetworkingObjects::NetObjID, size_t)> getObjInListAtIndex;
            std::function<size_t(const ParentObjectIDPair&)> getIndexOfObjInList;
            std::function<void(NetworkingObjects::NetObjID, bool)> setDirectoryOpen;
            std::function<bool(NetworkingObjects::NetObjID)> drawNonDirectoryObjIconGUI;
            std::function<bool(const ParentObjectIDPair&)> drawObjGUI;
            std::function<void(NetworkingObjects::NetObjID, size_t, const std::vector<ParentObjectIDPair>&)> moveObjectsToListAtIndex;
        };

        struct SelectionData {
            std::vector<ParentObjectIDPair> orderedObjsSelected;
            std::unordered_set<NetworkingObjects::NetObjID> objsSelected;
        };

        void update(UpdateInputData& io, GUIManager& gui, NetworkingObjects::NetObjID rootObjID, const DisplayData& displayData, SelectionData& selectionData);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override;
    private:
        bool dragHoldSelected = false;

        void set_single_selected_object(SelectionData& selectionData);

        void recursive_gui(UpdateInputData& io, GUIManager& gui, const DisplayData& displayData, SelectionData& selectionData, NetworkingObjects::NetObjID objID, size_t& itemCount, size_t itemCountToStartAt, size_t itemCountToEndAt, size_t listDepth, std::optional<ParentObjectIDStack>& hoveredObject, std::vector<NetworkingObjects::NetObjID>& parentIDStack, bool parentJustSelected);

        void select_and_unselect_parents(const std::vector<NetworkingObjects::NetObjID>& parentIDStack, NetworkingObjects::NetObjID newSelectedObj, SelectionData& selectionData);

        void unselect_object(NetworkingObjects::NetObjID id, SelectionData& selectionData);

        bool topHalfOfHovered = false;
        std::optional<ParentObjectIDPair> objDragged;
        std::optional<ParentObjectIDStack> oldHoveredObject;
        std::chrono::steady_clock::time_point timeStartedHoveringOverObject;
};

}
