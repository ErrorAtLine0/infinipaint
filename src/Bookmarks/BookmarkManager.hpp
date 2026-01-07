#pragma once
#include <Helpers/Serializers.hpp>
#include <cereal/types/string.hpp>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "../GUIStuff/Elements/TreeListing.hpp"
#include "BookmarkListItem.hpp"
#include <Helpers/VersionNumber.hpp>

class World;

class BookmarkManager {
    public:
        BookmarkManager(World& w);
        void server_init_no_file();
        void scale_up(const WorldScalar& scaleUpAmount);
        void setup_list_gui(const char* id);
        void refresh_gui_data();
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);

        NetworkingObjects::NetObjOwnerPtr<BookmarkListItem> bookmarkListRoot;
    private:
        std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem>> create_in_proper_position(BookmarkListItem* newItem);
        std::optional<std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem>>> try_to_create_in_proper_position(BookmarkListItem* newItem);
        NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem> create_bookmark(BookmarkListItem* newItem);
        void remove_bookmark(const NetworkingObjects::NetObjID& parentID, const NetworkingObjects::NetObjID& objectID);

        GUIStuff::TreeListing::SelectionData selectionData;
        std::unordered_set<NetworkingObjects::NetObjID> oldSelection;
        std::string nameToEdit;
        std::string nameForNew;

        std::optional<std::string> editingBookmarkName;

        void editing_bookmark_check();

        friend class BookmarkListItem;

        World& world;
};
