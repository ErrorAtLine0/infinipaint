#include "BookmarkManager.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "Helpers/NetworkingObjects/NetObjOrderedList.hpp"
#include "World.hpp"
#include "Server/CommandList.hpp"
#include "MainProgram.hpp"
#include <algorithm>

void Bookmark::scale_up(const WorldScalar& scaleUpAmount) {
    coords.scale_about(WorldVec{0, 0}, scaleUpAmount, true);
}

BookmarkManager::BookmarkManager(World& w):
    world(w)
{
    if(w.netObjMan.is_server())
        bookmarks = w.netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<Bookmark>>();
}

void BookmarkManager::add_bookmark(const std::string& name) {
    NetworkingObjects::NetObjPtr<Bookmark> a = bookmarks->emplace_back_direct(bookmarks, Bookmark{name, world.drawData.cam.c, world.main.window.size.cast<int32_t>()});
}

void BookmarkManager::remove_bookmark(uint32_t bookmarkIndex) {
    bookmarks->erase(bookmarks, bookmarkIndex);
}

void BookmarkManager::jump_to_bookmark(uint32_t bookmarkIndex) {
    const NetworkingObjects::NetObjPtr<Bookmark>& b = bookmarks->at(bookmarkIndex);
    world.drawData.cam.smooth_move_to(world, b->coords, b->windowSize.cast<float>());
}

void BookmarkManager::scale_up(const WorldScalar& scaleUpAmount) {
    for(uint32_t i = 0; i < bookmarks->size(); i++)
        bookmarks->at(i)->scale_up(scaleUpAmount);
}
