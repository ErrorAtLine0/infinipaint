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
        void read_create_message(cereal::PortableBinaryInputArchive& a);
        void scale_up(const WorldScalar& scaleUpAmount);
        void setup_list_gui();
        void refresh_gui_data();
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);

        NetworkingObjects::NetObjOwnerPtr<BookmarkListItem> bookmarkListRoot;
    private:
        NetworkingObjects::NetObjTemporaryPtr<BookmarkListItem> get_bookmark_parent_from_obj_index(const GUIStuff::TreeListingObjIndexList& objIndex);
        NetworkingObjects::NetObjTemporaryPtr<BookmarkListItem> get_bookmark_from_obj_index(const GUIStuff::TreeListingObjIndexList& objIndex);
        std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem>> create_in_proper_position(BookmarkListItem* newItem);
        std::optional<std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem>>> try_to_create_in_proper_position(BookmarkListItem* newItem);
        NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem> create_bookmark(BookmarkListItem* newItem);
        void remove_bookmark(const GUIStuff::TreeListingObjIndexList& objIndex);

        std::string nameToEdit;
        std::string nameForNew;

        std::set<GUIStuff::TreeListingObjIndexList> selectedBookmarkIndices;

        NetworkingObjects::NetObjWeakPtr<BookmarkListItem> editingBookmark;
        std::optional<std::string> editingBookmarkOldName;

        void editing_bookmark_check();

        friend class BookmarkListItem;

        World& world;
};
