#pragma once
#include <Helpers/Serializers.hpp>
#include <cereal/types/string.hpp>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "BookmarkListItem.hpp"

class World;

class BookmarkManager {
    public:
        BookmarkManager(World& w);
        void init();
        void scale_up(const WorldScalar& scaleUpAmount);
        void setup_list_gui(const std::string& id);

        template <typename Archive> void save(Archive& a) const {
        }

        template <typename Archive> void load(Archive& a) {
        }

        NetworkingObjects::NetObjOwnerPtr<BookmarkListItem> bookmarkListRoot;
    private:
        World& world;
};
