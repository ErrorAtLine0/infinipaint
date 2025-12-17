#include "BookmarkManager.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "Helpers/NetworkingObjects/NetObjOrderedList.hpp"
#include "World.hpp"
#include "MainProgram.hpp"
#include <algorithm>

void Bookmark::scale_up(const WorldScalar& scaleUpAmount) {
    coords.scale_about(WorldVec{0, 0}, scaleUpAmount, true);
}

BookmarkManager::BookmarkManager(World& w):
    world(w)
{}

void BookmarkManager::init() {
    // NOTE: Should find a way for bookmarks added from the network to take into account the client's canvasScale
    // Have the canvasScale variable come with the ClientData object, and the grid scale commands can be part of 
    // the client data class
    if(world.netObjMan.is_server())
        bookmarks = world.netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<Bookmark>>();
}

void BookmarkManager::add_bookmark(const std::string& name) {
    bookmarks->emplace_back_direct(bookmarks, Bookmark{name, world.drawData.cam.c, world.main.window.size.cast<int32_t>()});
}

void BookmarkManager::remove_bookmark(uint32_t bookmarkIndex) {
    bookmarks->erase(bookmarks, bookmarkIndex);
}

void BookmarkManager::jump_to_bookmark(uint32_t bookmarkIndex) {
    auto& b = bookmarks->at(bookmarkIndex);
    world.drawData.cam.smooth_move_to(world, b->obj->coords, b->obj->windowSize.cast<float>());
}

void BookmarkManager::scale_up(const WorldScalar& scaleUpAmount) {
    for(uint32_t i = 0; i < bookmarks->size(); i++)
        bookmarks->at(i)->obj->scale_up(scaleUpAmount);
}
