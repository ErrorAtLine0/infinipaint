#include "BookmarkManager.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "World.hpp"
#include "Server/CommandList.hpp"
#include "MainProgram.hpp"
#include <algorithm>

void Bookmark::scale_up(const WorldScalar& scaleUpAmount) {
    coords.scale_about(WorldVec{0, 0}, scaleUpAmount, true);
}

BookmarkManager::BookmarkManager(World& w):
    world(w)
{}

void BookmarkManager::init_client_callbacks() {
    world.con.client_add_recv_callback(CLIENT_NEW_BOOKMARK, [&](cereal::PortableBinaryInputArchive& message) {
        std::string bName;
        message(bName);
        auto& b = bookmarks[bName];
        message(b);
        changed = true;
    });
    world.con.client_add_recv_callback(CLIENT_REMOVE_BOOKMARK, [&](cereal::PortableBinaryInputArchive& message) {
        std::string name;
        message(name);
        bookmarks.erase(name);
        changed = true;
    });
}

void BookmarkManager::add_bookmark(const std::string& name) {
    Bookmark b{world.drawData.cam.c, world.main.window.size.cast<int32_t>()};
    bookmarks[name] = b;
    world.con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_NEW_BOOKMARK, name, b);
    changed = true;
}

void BookmarkManager::remove_bookmark(const std::string& name) {
    bookmarks.erase(name);
    world.con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_REMOVE_BOOKMARK, name);
    changed = true;
}

void BookmarkManager::jump_to_bookmark(const std::string& name) {
    auto it = bookmarks.find(name);
    if(it != bookmarks.end()) {
        auto& b = it->second;
        world.drawData.cam.smooth_move_to(world, b.coords, b.windowSize.cast<float>());
    }
}

const std::vector<std::string>& BookmarkManager::sorted_names() {
    if(changed) {
        sortedNames.clear();
        for(auto& [k, v] : bookmarks)
            sortedNames.emplace_back(k);
        std::sort(sortedNames.begin(), sortedNames.end());
        changed = false;
    }
    return sortedNames;
}

const std::unordered_map<std::string, Bookmark>& BookmarkManager::bookmark_list() {
    return bookmarks;
}

void BookmarkManager::scale_up(const WorldScalar& scaleUpAmount) {
    for(auto& [n, b] : bookmarks)
        b.scale_up(scaleUpAmount);
}
