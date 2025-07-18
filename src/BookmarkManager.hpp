#pragma once
#include <Helpers/Serializers.hpp>
#include <cereal/types/string.hpp>
#include "SharedTypes.hpp"
#include "CoordSpaceHelper.hpp"

struct Bookmark {
    CoordSpaceHelper coords;
    Vector<int32_t, 2> windowSize;
    template <class Archive> void serialize(Archive& a) {
        a(coords, windowSize);
    }
};

class World;

class BookmarkManager {
    public:
        BookmarkManager(World& w);
        void init_client_callbacks();
        void add_bookmark(const std::string& name);
        void remove_bookmark(const std::string& name);
        void jump_to_bookmark(const std::string& name);
        const std::unordered_map<std::string, Bookmark>& bookmark_list();
        template <typename Archive> void serialize(Archive& a) {
            a(bookmarks);
        }
        const std::vector<std::string>& sorted_names();
    private:
        bool changed = true; 
        std::vector<std::string> sortedNames;
        std::unordered_map<std::string, Bookmark> bookmarks;
        World& world;
};
