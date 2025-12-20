#include "BookmarkManager.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"

BookmarkManager::BookmarkManager(World& w):
    world(w)
{}

void BookmarkManager::init() {
    // NOTE: Should find a way for bookmarks added from the network to take into account the client's canvasScale
    // Have the canvasScale variable come with the ClientData object, and the grid scale commands can be part of 
    // the client data class
    if(world.netObjMan.is_server()) {
        bookmarkListRoot = world.netObjMan.make_obj_from_ptr<BookmarkListItem>(new BookmarkListItem(world.netObjMan, "ROOT", true, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 1", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 2", false, {}));
        auto folder1Ptr = bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Folder 1", true, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark A", false, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark B", false, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark C", false, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark D", false, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark E", false, {}));
            auto folderAPtr = folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Folder A", true, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 1", false, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 2", false, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 3", false, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 4", false, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 5", false, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 6", false, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 7", false, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark F", false, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark G", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 3", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 4", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 5", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 6", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 7", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 8", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 9", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 10", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 11", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 12", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 13", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 14", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 15", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 16", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 17", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 18", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 19", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 20", false, {}));
    }
}

void BookmarkManager::scale_up(const WorldScalar& scaleUpAmount) {
}
