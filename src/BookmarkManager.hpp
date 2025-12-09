#pragma once
#include <Helpers/Serializers.hpp>
#include <cereal/types/string.hpp>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "SharedTypes.hpp"
#include "CoordSpaceHelper.hpp"

struct Bookmark {
    std::string name;
    CoordSpaceHelper coords;
    Vector<int32_t, 2> windowSize;
    template <class Archive> void serialize(Archive& a) {
        a(name, coords, windowSize);
    }
    void scale_up(const WorldScalar& scaleUpAmount);
};

class World;

class BookmarkManager {
    public:
        BookmarkManager(World& w);
        void add_bookmark(const std::string& name);
        void remove_bookmark(uint32_t bookmarkIndex);
        void jump_to_bookmark(uint32_t bookmarkIndex);
        void scale_up(const WorldScalar& scaleUpAmount);

        template <typename Archive> void save(Archive& a) const {
            a(bookmarks->size());
            for(uint32_t i = 0; i < bookmarks->size(); i++)
                a(*bookmarks->at(i));
        }

        template <typename Archive> void load(Archive& a) {
            uint32_t newSize = bookmarks->size();
            for(uint32_t i = 0; i < newSize; i++) {
                Bookmark b;
                a(b);
                bookmarks->emplace_back_direct(bookmarks, b);
            }
        }

        NetworkingObjects::NetObjPtr<NetworkingObjects::NetObjOrderedList<Bookmark>> bookmarks;
    private:
        World& world;
};
