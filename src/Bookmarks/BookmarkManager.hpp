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
        void setup_list_gui(const std::string& id);
        void refresh_gui_data();
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);

        NetworkingObjects::NetObjOwnerPtr<BookmarkListItem> bookmarkListRoot;
    private:
        NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem> create_in_proper_position(BookmarkListItem* newItem);
        std::optional<NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem>> try_to_create_in_proper_position(BookmarkListItem* newItem);
        GUIStuff::TreeListing::SelectionData selectionData;
        std::unordered_set<NetworkingObjects::NetObjID> oldSelection;
        std::string nameToEdit;
        std::string nameForNew;

        friend class BookmarkListItem;

        World& world;
};
