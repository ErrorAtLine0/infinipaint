#pragma once
#include <Helpers/Serializers.hpp>
#include <cereal/types/string.hpp>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "../GUIStuff/Elements/TreeListing.hpp"
#include "BookmarkListItem.hpp"

class World;

class BookmarkManager {
    public:
        BookmarkManager(World& w);
        void init();
        void scale_up(const WorldScalar& scaleUpAmount);
        void setup_list_gui(const std::string& id);
        void refresh_gui_data();

        template <typename Archive> void save(Archive& a) const {
        }

        template <typename Archive> void load(Archive& a) {
        }

        NetworkingObjects::NetObjOwnerPtr<BookmarkListItem> bookmarkListRoot;
    private:
        NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem> create_in_proper_position(BookmarkListItem* newItem);
        std::optional<NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem>> try_to_create_in_proper_position(BookmarkListItem* newItem);
        GUIStuff::TreeListing::SelectionData selectionData;
        std::unordered_set<NetworkingObjects::NetObjID> oldSelection;
        std::string nameToEdit;
        std::string nameForNew;

        World& world;
};
